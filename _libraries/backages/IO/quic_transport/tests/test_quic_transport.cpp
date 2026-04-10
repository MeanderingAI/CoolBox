#include "quic_transport.hpp"
#include "tyst_framework.hpp"

TYST_TEST(QuicTransportTest, SessionQueuesRequestsAndResponsesThroughDatagrams) {
    networking::quic::Session session(networking::quic::SessionRole::Server);
    session.begin_handshake("peer-a");
    TYST_ASSERT_EQ(session.activate(), true);

    const std::uint64_t stream_id = session.open_stream();
    networking::quic::Http3Request request;
    request.stream_id = stream_id;
    request.method = "POST";
    request.path = "/submit";
    request.headers.push_back({"Content-Type", "application/json"});
    request.body = "{\"value\":1}";

    TYST_ASSERT_EQ(session.send_request(request), true);
    TYST_ASSERT_EQ(session.has_outbound_datagrams(), true);

    const auto datagram = session.pop_outbound_datagram();
    TYST_ASSERT_EQ(session.receive_datagram(datagram), true);
    TYST_ASSERT_EQ(session.has_pending_requests(), true);

    const auto decoded_request = session.pop_request();
    TYST_EXPECT_EQ(decoded_request.method, "POST");
    TYST_EXPECT_EQ(decoded_request.path, "/submit");
    TYST_EXPECT_EQ(decoded_request.body, "{\"value\":1}");

    networking::quic::Http3Response response;
    response.stream_id = stream_id;
    response.status_code = 201;
    response.headers.push_back({"Content-Type", "application/json"});
    response.body = "{\"ok\":true}";

    TYST_ASSERT_EQ(session.send_response(response), true);
    TYST_ASSERT_EQ(session.receive_datagram(session.pop_outbound_datagram()), true);
    TYST_ASSERT_EQ(session.has_pending_responses(), true);
    const auto decoded_response = session.pop_response();
    TYST_EXPECT_EQ(decoded_response.status_code, 201);
    TYST_EXPECT_EQ(decoded_response.body, "{\"ok\":true}");
}

TYST_TEST(QuicTransportTest, ConvertsBetweenHttpAndHttp3Structures) {
    Request request;
    request.method = "GET";
    request.uri = "/health";
    request.headers[HeaderKey::Accept] = "application/json";

    const auto http3_request = networking::quic::request_from_http1(request, 8);
    TYST_EXPECT_EQ(http3_request.stream_id, 8U);
    TYST_EXPECT_EQ(http3_request.method, "GET");
    TYST_EXPECT_EQ(http3_request.path, "/health");

    const auto request_roundtrip = networking::quic::request_to_http1(http3_request);
    TYST_EXPECT_EQ(request_roundtrip.method, "GET");
    TYST_EXPECT_EQ(request_roundtrip.uri, "/health");

    Response response = Response::ok("ready");
    response.headers[HeaderKey::ContentType] = "text/plain";

    const auto http3_response = networking::quic::response_from_http1(response, 8);
    TYST_EXPECT_EQ(http3_response.stream_id, 8U);
    TYST_EXPECT_EQ(http3_response.status_code, 200);
    TYST_EXPECT_EQ(http3_response.body, "ready");

    const auto response_roundtrip = networking::quic::response_to_http1(http3_response);
    TYST_EXPECT_EQ(response_roundtrip.status_code, 200);
    TYST_EXPECT_EQ(response_roundtrip.body, "ready");
}

TYST_TEST(QuicTransportTest, RouteRegistryDispatchesRegisteredAndFallbackHandlers) {
    networking::quic::RouteRegistry routes;
    routes.add_route("/health", [](const networking::quic::Http3Request& request) {
        networking::quic::Http3Response response;
        response.stream_id = request.stream_id;
        response.status_code = 200;
        response.body = "healthy";
        return response;
    });
    routes.set_fallback([](const networking::quic::Http3Request& request) {
        networking::quic::Http3Response response;
        response.stream_id = request.stream_id;
        response.status_code = 418;
        response.body = request.path;
        return response;
    });

    networking::quic::Http3Request request;
    request.stream_id = 4;
    request.path = "/health";
    const auto health = routes.dispatch(request);
    TYST_EXPECT_EQ(health.status_code, 200);
    TYST_EXPECT_EQ(health.body, "healthy");

    request.path = "/missing";
    const auto fallback = routes.dispatch(request);
    TYST_EXPECT_EQ(fallback.status_code, 418);
    TYST_EXPECT_EQ(fallback.body, "/missing");
}

TYST_TEST(QuicTransportTest, EndpointsPerformLoopbackExchangeThroughRoutes) {
    networking::quic::LoopbackExchange exchange;
    exchange.routes().add_route("/sum", [](const networking::quic::Http3Request& request) {
        networking::quic::Http3Response response;
        response.stream_id = request.stream_id;
        response.status_code = 200;
        response.headers.push_back({"content-type", "text/plain"});
        response.body = "sum=" + request.body;
        return response;
    });

    networking::quic::Http3Request request;
    request.method = "POST";
    request.path = "/sum";
    request.body = "3+4";

    const auto response = exchange.perform(request);
    TYST_EXPECT_EQ(response.status_code, 200);
    TYST_EXPECT_EQ(response.body, "sum=3+4");
    TYST_EXPECT_GE(exchange.client().session().outbound_datagram_count(), 0U);
}