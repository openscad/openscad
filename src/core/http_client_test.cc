#include "core/HTTPClient.h"

#include <catch2/catch_all.hpp>

TEST_CASE("parseURL handles bracketed IPv6 literal with port", "[HTTPClient][IPv6]")
{
  ParsedURL p = parseURL("http://[::1]:8080/v1");
  REQUIRE(p.error.empty());
  CHECK(p.scheme == "http");
  CHECK(p.host == "::1");
  CHECK(p.port == 8080);
  CHECK(p.target == "/v1");
  CHECK(p.ipv6);
  CHECK(p.host_header == "[::1]:8080");
}

TEST_CASE("parseURL handles bracketed IPv6 literal with default port", "[HTTPClient][IPv6]")
{
  ParsedURL p = parseURL("http://[::1]/v1");
  REQUIRE(p.error.empty());
  CHECK(p.host == "::1");
  CHECK(p.port == 80);
  CHECK(p.target == "/v1");
  CHECK(p.ipv6);
  CHECK(p.host_header == "[::1]");
}

TEST_CASE("parseURL handles full IPv6 address with https default port", "[HTTPClient][IPv6]")
{
  ParsedURL p = parseURL("https://[2001:db8::1]/");
  REQUIRE(p.error.empty());
  CHECK(p.scheme == "https");
  CHECK(p.host == "2001:db8::1");
  CHECK(p.port == 443);
  CHECK(p.target == "/");
  CHECK(p.ipv6);
  CHECK(p.host_header == "[2001:db8::1]");
}

TEST_CASE("parseURL handles IPv6 literal with explicit https port", "[HTTPClient][IPv6]")
{
  ParsedURL p = parseURL("https://[2001:db8::1]:8443/api");
  REQUIRE(p.error.empty());
  CHECK(p.host == "2001:db8::1");
  CHECK(p.port == 8443);
  CHECK(p.target == "/api");
  CHECK(p.ipv6);
  CHECK(p.host_header == "[2001:db8::1]:8443");
}

TEST_CASE("parseURL omits explicit default port for IPv6", "[HTTPClient][IPv6]")
{
  ParsedURL p = parseURL("http://[::1]:80/v1");
  REQUIRE(p.error.empty());
  CHECK(p.port == 80);
  CHECK(p.host_header == "[::1]");
}

TEST_CASE("parseURL rejects IPv6 literal without closing bracket", "[HTTPClient][IPv6][error]")
{
  ParsedURL p = parseURL("http://[::1/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.host.empty());
}

TEST_CASE("parseURL rejects empty IPv6 literal", "[HTTPClient][IPv6][error]")
{
  ParsedURL p = parseURL("http://[]:8080/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.host.empty());
}

TEST_CASE("parseURL rejects junk after closing bracket", "[HTTPClient][IPv6][error]")
{
  ParsedURL p = parseURL("http://[::1]junk/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.host.empty());
}

TEST_CASE("parseURL rejects unbracketed IPv6 literal", "[HTTPClient][IPv6][error]")
{
  ParsedURL p = parseURL("http://::1/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.host.empty());
}

TEST_CASE("parseURL keeps hostname with port", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("http://localhost:11434/v1");
  REQUIRE(p.error.empty());
  CHECK(p.host == "localhost");
  CHECK(p.port == 11434);
  CHECK(p.target == "/v1");
  CHECK_FALSE(p.ipv6);
  CHECK(p.host_header == "localhost:11434");
}

TEST_CASE("parseURL keeps IPv4 address with port", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("http://127.0.0.1:8080/v1");
  REQUIRE(p.error.empty());
  CHECK(p.host == "127.0.0.1");
  CHECK(p.port == 8080);
  CHECK(p.target == "/v1");
  CHECK_FALSE(p.ipv6);
  CHECK(p.host_header == "127.0.0.1:8080");
}

TEST_CASE("parseURL defaults https port to 443", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("https://openscad.org/v1");
  REQUIRE(p.error.empty());
  CHECK(p.host == "openscad.org");
  CHECK(p.port == 443);
  CHECK(p.target == "/v1");
  CHECK(p.host_header == "openscad.org");
}

TEST_CASE("parseURL defaults http port to 80", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("http://localhost/v1");
  REQUIRE(p.error.empty());
  CHECK(p.host == "localhost");
  CHECK(p.port == 80);
  CHECK(p.target == "/v1");
  CHECK(p.host_header == "localhost");
}

TEST_CASE("parseURL omits explicit default port from host header", "[HTTPClient][regression]")
{
  {
    ParsedURL p = parseURL("http://localhost:80/v1");
    REQUIRE(p.error.empty());
    CHECK(p.port == 80);
    CHECK(p.host_header == "localhost");
  }
  {
    ParsedURL p = parseURL("https://openscad.org:443/v1");
    REQUIRE(p.error.empty());
    CHECK(p.port == 443);
    CHECK(p.host_header == "openscad.org");
  }
  {
    ParsedURL p = parseURL("http://127.0.0.1:80/v1");
    REQUIRE(p.error.empty());
    CHECK(p.host_header == "127.0.0.1");
  }
  {
    ParsedURL p = parseURL("https://127.0.0.1:443/v1");
    REQUIRE(p.error.empty());
    CHECK(p.host_header == "127.0.0.1");
  }
}

TEST_CASE("parseURL lowercases the scheme", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("HTTPS://OPENSCAD.ORG/V1");
  REQUIRE(p.error.empty());
  CHECK(p.scheme == "https");
  CHECK(p.host == "OPENSCAD.ORG");
  CHECK(p.port == 443);
  CHECK(p.target == "/V1");
  CHECK(p.host_header == "OPENSCAD.ORG");
}

TEST_CASE("parseURL defaults to https when no scheme is given", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("openscad.org/v1");
  REQUIRE(p.error.empty());
  CHECK(p.scheme == "https");
  CHECK(p.host == "openscad.org");
  CHECK(p.port == 443);
  CHECK(p.target == "/v1");
}

TEST_CASE("parseURL defaults target to / when no path is given", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("http://localhost");
  REQUIRE(p.error.empty());
  CHECK(p.host == "localhost");
  CHECK(p.port == 80);
  CHECK(p.target == "/");
  CHECK(p.host_header == "localhost");
}

TEST_CASE("parseURL keeps query string in target", "[HTTPClient][regression]")
{
  ParsedURL p = parseURL("http://localhost:11434/v1?x=1&y=2");
  REQUIRE(p.error.empty());
  CHECK(p.target == "/v1?x=1&y=2");
  CHECK(p.host_header == "localhost:11434");
}

TEST_CASE("parseURL extracts userinfo with password", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("http://user:pass@example.com/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo == "user:pass");
  CHECK(p.host == "example.com");
  CHECK(p.port == 80);
  CHECK(p.target == "/v1");
  CHECK(p.host_header == "example.com");
}

TEST_CASE("parseURL extracts userinfo without password", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("http://user@example.com/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo == "user");
  CHECK(p.host == "example.com");
}

TEST_CASE("parseURL keeps port with userinfo", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("https://user:pass@example.com:8443/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo == "user:pass");
  CHECK(p.host == "example.com");
  CHECK(p.port == 8443);
  CHECK(p.host_header == "example.com:8443");
}

TEST_CASE("parseURL handles empty userinfo", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("http://@example.com/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo.empty());
  CHECK(p.host == "example.com");
}

TEST_CASE("parseURL splits userinfo at last @", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("http://a@b@example.com/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo == "a@b");
  CHECK(p.host == "example.com");
}

TEST_CASE("parseURL handles userinfo with bracketed IPv6", "[HTTPClient][userinfo][IPv6]")
{
  ParsedURL p = parseURL("http://user:pass@[::1]:8080/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo == "user:pass");
  CHECK(p.host == "::1");
  CHECK(p.port == 8080);
  CHECK(p.ipv6);
  CHECK(p.host_header == "[::1]:8080");
}

TEST_CASE("parseURL keeps userinfo empty when no @ in authority", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("http://example.com/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo.empty());
  CHECK(p.host == "example.com");
}

TEST_CASE("parseURL ignores @ in path", "[HTTPClient][userinfo]")
{
  ParsedURL p = parseURL("http://example.com/@tag/v1");
  REQUIRE(p.error.empty());
  CHECK(p.userinfo.empty());
  CHECK(p.host == "example.com");
  CHECK(p.target == "/@tag/v1");
}

TEST_CASE("parseURL rejects userinfo with empty host", "[HTTPClient][userinfo][error]")
{
  ParsedURL p = parseURL("http://user:@/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.host.empty());
}

TEST_CASE("parseURL rejects non-numeric port", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http://localhost:abc/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.port == 0);
}

TEST_CASE("parseURL rejects port with trailing junk", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http://localhost:80a/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.port == 0);
}

TEST_CASE("parseURL rejects out-of-range port", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http://localhost:99999/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.port == 0);
}

TEST_CASE("parseURL rejects zero port", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http://localhost:0/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.port == 0);
}

TEST_CASE("parseURL rejects negative port", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http://localhost:-1/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.port == 0);
}

TEST_CASE("parseURL accepts boundary ports", "[HTTPClient][regression]")
{
  {
    ParsedURL p = parseURL("http://localhost:1/v1");
    REQUIRE(p.error.empty());
    CHECK(p.port == 1);
    CHECK(p.host_header == "localhost:1");
  }
  {
    ParsedURL p = parseURL("http://localhost:65535/v1");
    REQUIRE(p.error.empty());
    CHECK(p.port == 65535);
    CHECK(p.host_header == "localhost:65535");
  }
}

TEST_CASE("parseURL rejects empty port", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http://localhost:/v1");
  CHECK_FALSE(p.error.empty());
  CHECK(p.port == 0);
}

TEST_CASE("parseURL rejects unsupported scheme", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("ftp://example.com/v1");
  CHECK_FALSE(p.error.empty());
}

TEST_CASE("parseURL rejects empty host", "[HTTPClient][error]")
{
  ParsedURL p = parseURL("http:///v1");
  CHECK_FALSE(p.error.empty());
}
