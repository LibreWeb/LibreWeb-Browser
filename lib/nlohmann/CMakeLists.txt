# Vendored single-header nlohmann/json (was previously provided transitively by the
# now-removed cpp-ipfs-http-client submodule). Exposed as an interface target so the
# existing `nlohmann_json::nlohmann_json` link name keeps working.
add_library(nlohmann_json INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
