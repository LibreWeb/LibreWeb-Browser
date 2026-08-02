#include "freedomnames.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{
  /**
   * \brief Minimal one-shot HTTP server that returns a canned response.
   *
   * Lets the tests drive FreedomNames against bodies a real node would never
   * send: non-JSON, wrong-typed fields, an empty body. Binds port 0 so the
   * kernel picks a free port and parallel test runs cannot collide.
   */
  class OneShotHttpServer
  {
  public:
    explicit OneShotHttpServer(const std::string& body, const std::string& status = "200 OK")
    {
      listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
      int reuse = 1;
      setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      addr.sin_port = 0; // let the kernel choose
      bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
      listen(listen_fd_, 1);

      socklen_t len = sizeof(addr);
      getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
      port_ = ntohs(addr.sin_port);

      thread_ = std::thread(
          [this, body, status]()
          {
            int client = accept(listen_fd_, nullptr, nullptr);
            if (client < 0)
              return;
            char scratch[4096];
            ssize_t ignored = recv(client, scratch, sizeof(scratch), 0); // drain the request
            (void)ignored;
            const std::string response = "HTTP/1.1 " + status +
                                         "\r\nContent-Type: application/json"
                                         "\r\nContent-Length: " +
                                         std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
            ssize_t sent = send(client, response.c_str(), response.size(), 0);
            (void)sent;
            close(client);
          });
    }

    ~OneShotHttpServer()
    {
      if (thread_.joinable())
        thread_.join();
      if (listen_fd_ >= 0)
        close(listen_fd_);
    }

    int port() const
    {
      return port_;
    }

  private:
    int listen_fd_ = -1;
    int port_ = 0;
    std::thread thread_;
  };

  // A well-formed /health response is parsed into every field, role included.
  TEST(FreedomNamesTest, HealthParsesWellFormedResponse)
  {
    OneShotHttpServer server(R"({"status":"ok","version":"0.8.4","ready":true,"role":"bootstrap"})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    FreedomHealth health = client.get_health();

    EXPECT_EQ(health.version, "0.8.4");
    EXPECT_TRUE(health.ready);
    EXPECT_EQ(health.role, "bootstrap");
  }

  // A node older than 0.8.4 omits "role" entirely; it must parse, not throw.
  TEST(FreedomNamesTest, HealthWithoutRoleLeavesRoleEmpty)
  {
    OneShotHttpServer server(R"({"status":"ok","version":"0.8.3","ready":true})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    FreedomHealth health = client.get_health();

    EXPECT_EQ(health.version, "0.8.3");
    EXPECT_TRUE(health.role.empty());
  }

  // A 2xx response whose body is not JSON at all. nlohmann's parse_error derives
  // from std::exception rather than std::runtime_error, so without the
  // translation in freedomnames.cc this escaped every caller's catch and
  // terminated the browser (observed: exit 134, "terminate called after
  // throwing an instance of ... parse_error").
  TEST(FreedomNamesTest, HealthThrowsRuntimeErrorOnNonJsonBody)
  {
    OneShotHttpServer server("<html>not json at all</html>");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_THROW(client.get_health(), std::runtime_error);
  }

  // A key present with the wrong type: json::value() throws type_error, which
  // also derives from std::exception and not std::runtime_error.
  TEST(FreedomNamesTest, HealthThrowsRuntimeErrorOnWrongTypedField)
  {
    OneShotHttpServer server(R"({"status":"ok","version":"0.8.4","ready":"yes","role":"node"})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_THROW(client.get_health(), std::runtime_error);
  }

  // An empty 2xx body is also a parse error, not a crash.
  TEST(FreedomNamesTest, HealthThrowsRuntimeErrorOnEmptyBody)
  {
    OneShotHttpServer server("");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_THROW(client.get_health(), std::runtime_error);
  }

  // Same contract on /info, which the status popover polls repeatedly.
  TEST(FreedomNamesTest, InfoThrowsRuntimeErrorOnNonJsonBody)
  {
    OneShotHttpServer server("not json");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_THROW(client.get_info(), std::runtime_error);
  }

  // "hostsConnected" as a string rather than a number.
  TEST(FreedomNamesTest, InfoThrowsRuntimeErrorOnWrongTypedField)
  {
    OneShotHttpServer server(R"({"mode":"Auto","peerID":"12D3Koo","hostsConnected":"many"})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_THROW(client.get_info(), std::runtime_error);
  }

  // The status pop-over reads the counts and the version straight off /info.
  TEST(FreedomNamesTest, InfoParsesListsAndVersion)
  {
    OneShotHttpServer server(R"({"version":"0.9.2","mode":"Auto","peerID":"12D3Koo","hostsConnected":2,"networkSize":7,)"
                             R"("peers":["12D3KooA","12D3KooB"],)"
                             R"("listenAddresses":["/ip4/127.0.0.1/tcp/4001","/ip4/10.0.0.2/udp/4001/quic-v1"],)"
                             R"("protocols":["/ipfs/id/1.0.0"]})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    FreedomInfo info = client.get_info();

    EXPECT_EQ(info.version, "0.9.2");
    EXPECT_EQ(info.peers, 2u);
    EXPECT_EQ(info.network_size, 7);
    // "peers" is the DHT routing table, which is not the connected-host count.
    ASSERT_EQ(info.routing_table.size(), 2u);
    EXPECT_EQ(info.routing_table[0], "12D3KooA");
    EXPECT_EQ(info.listen_addresses.size(), 2u);
    ASSERT_EQ(info.protocols.size(), 1u);
    EXPECT_EQ(info.protocols[0], "/ipfs/id/1.0.0");
  }

  // The lists are status decoration, so a junk entry costs only that entry, and
  // an absent field is simply empty. Neither may fail the whole status update.
  TEST(FreedomNamesTest, InfoSkipsNonStringListEntriesAndMissingLists)
  {
    OneShotHttpServer server(R"({"mode":"Auto","peerID":"12D3Koo","hostsConnected":0,)"
                             R"("peers":["12D3KooA",42,null,"12D3KooB"]})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    FreedomInfo info = client.get_info();

    ASSERT_EQ(info.routing_table.size(), 2u);
    EXPECT_EQ(info.routing_table[1], "12D3KooB");
    EXPECT_TRUE(info.listen_addresses.empty());
    EXPECT_TRUE(info.protocols.empty());
  }

  // A list field of the wrong type altogether is ignored rather than fatal.
  TEST(FreedomNamesTest, InfoIgnoresListFieldThatIsNotAnArray)
  {
    OneShotHttpServer server(R"({"mode":"Auto","peerID":"12D3Koo","hostsConnected":0,"protocols":"none"})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    FreedomInfo info = client.get_info();

    EXPECT_TRUE(info.protocols.empty());
  }

  // clear_cache() sends a DELETE and accepts the node's empty 200 body: there is
  // nothing to parse, so an empty body here must not be treated as an error.
  TEST(FreedomNamesTest, ClearCacheAcceptsEmptyBody)
  {
    OneShotHttpServer server("");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_NO_THROW(client.clear_cache());
  }

  // A node that refuses the call (eg. wrong method) still surfaces as an error.
  TEST(FreedomNamesTest, ClearCacheThrowsOnHttpError)
  {
    OneShotHttpServer server("Method not allowed", "405 Method Not Allowed");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    EXPECT_THROW(client.clear_cache(), std::runtime_error);
  }

  // A malformed record is dropped, but the well-formed ones still resolve --
  // one bad entry should not fail the whole lookup.
  TEST(FreedomNamesTest, ResolveSkipsMalformedRecords)
  {
    OneShotHttpServer server(R"({"records":[{"type":"A","value":"1.2.3.4","ttl":60},)"
                             R"("i-am-not-an-object",)"
                             R"({"type":"TXT","value":"hello","ttl":"soon"}]})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    std::vector<FreedomRecord> records = client.resolve("example.fn");

    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].type, "A");
    EXPECT_EQ(records[0].value, "1.2.3.4");
  }

  // Nothing listening on the port: a connection failure, still a runtime_error.
  TEST(FreedomNamesTest, HealthThrowsRuntimeErrorWhenNothingListening)
  {
    // Port 1 on loopback is refused immediately and is never a node.
    FreedomNames client("127.0.0.1", 1, "5s");

    EXPECT_THROW(client.get_health(), std::runtime_error);
  }
} // namespace
