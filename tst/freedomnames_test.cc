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

      std::string response_body = body;
      const std::string marker = "{PORT}";
      std::string::size_type at = 0;
      while ((at = response_body.find(marker, at)) != std::string::npos)
      {
        response_body.replace(at, marker.size(), std::to_string(port_));
        at += std::to_string(port_).size();
      }

      thread_ = std::thread(
          [this, response_body, status]()
          {
            int client = accept(listen_fd_, nullptr, nullptr);
            if (client < 0)
              return;
            char scratch[4096];
            while (request_.find("\r\n\r\n") == std::string::npos)
            {
              const ssize_t received = recv(client, scratch, sizeof(scratch), 0);
              if (received <= 0)
                break;
              request_.append(scratch, static_cast<std::size_t>(received));
            }
            const std::size_t headers_end = request_.find("\r\n\r\n");
            const std::size_t length_at = request_.find("Content-Length:");
            if (headers_end != std::string::npos && length_at != std::string::npos && length_at < headers_end)
            {
              const std::size_t value_at = length_at + std::string("Content-Length:").size();
              const std::size_t value_end = request_.find("\r\n", value_at);
              const std::size_t content_length = std::stoul(request_.substr(value_at, value_end - value_at));
              while (request_.size() < headers_end + 4 + content_length)
              {
                const ssize_t received = recv(client, scratch, sizeof(scratch), 0);
                if (received <= 0)
                  break;
                request_.append(scratch, static_cast<std::size_t>(received));
              }
            }
            const std::string response = "HTTP/1.1 " + status +
                                         "\r\nContent-Type: application/json"
                                         "\r\nContent-Length: " +
                                         std::to_string(response_body.size()) + "\r\nConnection: close\r\n\r\n" + response_body;
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

    std::string request()
    {
      if (thread_.joinable())
        thread_.join();
      return request_;
    }

  private:
    int listen_fd_ = -1;
    int port_ = 0;
    std::string request_;
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

  TEST(FreedomNamesTest, HealthDiscoversLocalAuthoringApi)
  {
    OneShotHttpServer server(
        R"({"status":"ok","version":"0.9.4","ready":true,"role":"node","capabilities":["authoring"],"authoringAPI":"http://127.0.0.1:{PORT}"})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    const FreedomHealth health = client.get_health();

    EXPECT_TRUE(health.has_capability("authoring"));
    EXPECT_EQ(health.authoring_api, "http://127.0.0.1:" + std::to_string(server.port()));
  }

  TEST(FreedomNamesTest, HealthDoesNotTrustNonLoopbackAuthoringApi)
  {
    OneShotHttpServer server(
        R"({"status":"ok","version":"0.9.4","ready":true,"role":"node","capabilities":["authoring"],"authoringAPI":"http://example.com:8421"})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    const FreedomHealth health = client.get_health();

    EXPECT_TRUE(health.has_capability("authoring"));
    EXPECT_TRUE(health.authoring_api.empty());
  }

  TEST(FreedomNamesTest, ListsOwnedNames)
  {
    OneShotHttpServer server(R"({"names":[{"label":"blog","name":"blog.owner.fn"},{"label":"damaged","name":""}]})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    const std::vector<FreedomName> names = client.list_names("http://127.0.0.1:" + std::to_string(server.port()));

    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names.at(0).label, "blog");
    EXPECT_EQ(names.at(0).name, "blog.owner.fn");
    EXPECT_EQ(names.at(1).label, "damaged");
  }

  TEST(FreedomNamesTest, CreatesNameWithJsonRequest)
  {
    OneShotHttpServer server(R"({"label":"blog","name":"blog.owner.fn"})", "201 Created");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    const std::string name = client.create_name("http://127.0.0.1:" + std::to_string(server.port()), "blog");
    const std::string request = server.request();

    EXPECT_EQ(name, "blog.owner.fn");
    EXPECT_NE(request.find("POST /authoring/names HTTP/1.1"), std::string::npos);
    EXPECT_NE(request.find("Content-Type: application/json"), std::string::npos);
    EXPECT_NE(request.find(R"({"label":"blog"})"), std::string::npos);
  }

  TEST(FreedomNamesTest, PublishesContentRecordWithJsonRequest)
  {
    OneShotHttpServer server(R"({"published":"blog.owner.fn","seq":4,"expires":123})");
    FreedomNames client("127.0.0.1", server.port(), "5s");

    const std::string name = client.publish_name("http://127.0.0.1:" + std::to_string(server.port()), "bad/label", "k2k4hash");
    const std::string request = server.request();

    EXPECT_EQ(name, "blog.owner.fn");
    EXPECT_NE(request.find("POST /authoring/names/bad%2Flabel/publish HTTP/1.1"), std::string::npos);
    EXPECT_NE(request.find(R"("records":[{"ttl":300,"type":"CONTENT","value":"k2k4hash"}])"), std::string::npos);
  }

  TEST(FreedomNamesTest, AuthoringRefusesUnsafeApiOrigin)
  {
    FreedomNames client("127.0.0.1", 8420, "5s");

    EXPECT_THROW(client.create_name("http://example.com:8421", "blog"), std::runtime_error);
    EXPECT_THROW(client.create_name("https://127.0.0.1:8421", "blog"), std::runtime_error);
    EXPECT_THROW(client.create_name("http://localhost:8421", "blog"), std::runtime_error);
    EXPECT_THROW(client.create_name("http://127.0.0.1:8421/path", "blog"), std::runtime_error);
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
