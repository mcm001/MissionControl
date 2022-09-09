#include <vector>

#include <wpi/span.h>

#include "gtest/gtest.h"
#include "wpinet/uv/Loop.h"
#include "wpinet/uv/Pipe.h"
#include "wpinet/uv/Tcp.h"
#include "wpinet/uv/Timer.h"

#include <wpinet/WebSocket.h> // NOLINT(build/include_order)
#include <wpi/StringExtras.h>
#include "wpinet/HttpParser.h"
#include <wpinet/WebSocketServer.h>
#include <wpinet/HttpServerConnection.h>

#include <iostream>
#include <fmt/format.h>

class MyHttpConnection : public wpi::HttpServerConnection,
                         public std::enable_shared_from_this<MyHttpConnection>
{
public:
    MyHttpConnection(std::shared_ptr<wpi::uv::Stream> stream);
    ~MyHttpConnection() = default;

      void ProcessRequest() override {}

    wpi::WebSocketServerHelper m_websocketHelper;
};

MyHttpConnection::MyHttpConnection(std::shared_ptr<wpi::uv::Stream> stream)
    : HttpServerConnection(stream), m_websocketHelper(m_request)
{
    // Handle upgrade event
    m_websocketHelper.upgrade.connect([this]
                                      {
    //fmt::print(stderr, "{}", "got websocket upgrade\n");
    // Disconnect HttpServerConnection header reader
    m_dataConn.disconnect();
    m_messageCompleteConn.disconnect();

    // Accepting the stream may destroy this (as it replaces the stream user
    // data), so grab a shared pointer first.
    auto self = shared_from_this();

    // Accept the upgrade
    auto ws = m_websocketHelper.Accept(m_stream, "frcvision");

    // Connect the websocket open event to our connected event.
    // Pass self to delay destruction until this callback happens
    ws->open.connect_extended([self, s = ws.get()](auto conn, auto) {
      fmt::print(stderr, "{}", "websocket connected\n");
      conn.disconnect();  // one-shot
    });
    ws->text.connect([s = ws.get()](auto msg, bool) {
    printf("Got ws text!");
    //   ProcessWsText(*s, msg);
    });
    ws->binary.connect([s = ws.get()](auto msg, bool) {
    //   ProcessWsBinary(*s, msg);
    printf("Got ws binary!");
    }); });
}

namespace uv = wpi::uv;
int main()
{
    auto loop = uv::Loop::Create();
    loop->error.connect(
        [](uv::Error err)
        { fmt::print(stderr, "uv ERROR: {}\n", err.str()); });
    auto tcp = uv::Tcp::Create(loop);
    tcp->Bind("127.0.0.1", 9002);
    // when we get a connection, accept it and start reading
    tcp->connection.connect([srv = tcp.get()]
                            {
    auto tcp = srv->Accept();
    if (!tcp) return;
    // fmt::print(stderr, "{}", "Got a connection\n");

    // Close on error
    tcp->error.connect([s = tcp.get()](wpi::uv::Error err) {
      fmt::print(stderr, "stream error: {}\n", err.str());
      s->Close();
    });

    auto conn = std::make_shared<MyHttpConnection>(tcp);
    tcp->SetData(conn); });

    // start listening for incoming connections
    tcp->Listen();

    printf("Running loop!");
    loop->Run();
}

// namespace wpi {

// namespace uv = wpi::uv;

// class WebSocketTest : public ::testing::Test {
//  public:
//   static const char* pipeName;
//   static const int port = 9002;

//   static void SetUpTestCase();

//   WebSocketTest() {
//     loop = uv::Loop::Create();
//     serverPipe = uv::Tcp::Create(loop);

//     serverPipe->Bind(pipeName, 9002);

//     auto failTimer = uv::Timer::Create(loop);
//     failTimer->timeout.connect([this] {
//       loop->Stop();
//       FAIL() << "loop failed to terminate";
//     });
//     failTimer->Start(uv::Timer::Time{20000});
//     failTimer->Unreference();

//     resp.headersComplete.connect([this](bool) { headersDone = true; });

//     serverPipe->Listen([this]() {
//       auto conn = serverPipe->Accept();
//       printf("Server got conn!\n");
//       ws = WebSocket::CreateServer(*conn, "/ws", "13");
//       if (setupWebSocket) {
//         setupWebSocket();
//       }
//     });
//   }

//   ~WebSocketTest() override {
//     Finish();
//   }

//   void Finish() {
//     loop->Walk([](uv::Handle& it) { it.Close(); });
//   }

//   std::shared_ptr<uv::Loop> loop;
//   std::shared_ptr<uv::Tcp> clientPipe;
//   std::shared_ptr<uv::Tcp> serverPipe;
//   std::function<void()> setupWebSocket;
//   std::function<void(std::string_view)> handleData;
//   std::vector<uint8_t> wireData;
//   std::shared_ptr<WebSocket> ws;
//   HttpParser resp{HttpParser::kResponse};
//   bool headersDone = false;
//   wpi::WebSocketServerHelper m_websocketHelper;
// };

// // #ifdef _WIN32
// // const char* WebSocketTest::pipeName = "\\\\.\\pipe\\websocket-unit-test";
// // #else
// // const char* WebSocketTest::pipeName = "/tmp/websocket-unit-test";
// // #endif
// const char* WebSocketTest::pipeName = "127.0.0.1";

// void WebSocketTest::SetUpTestCase() {
// #ifndef _WIN32
//   unlink(pipeName);
// #endif
// }

// TEST_F(WebSocketTest, SendText) {
//   int gotCallback = 0;
//   std::string str = "asdf";
//   std::vector<uint8_t> data = {str.begin(), str.end()};
//   setupWebSocket = [&] {
//     ws->open.connect([&](std::string_view) {
//       printf("WS Server got client!\n");
//       ws->SendText({{data}}, [&](auto bufs, uv::Error) {
//         printf("WS Server sent text!\n");
//         ++gotCallback;
//         // ws->Terminate();
//         ASSERT_FALSE(bufs.empty());
//         ASSERT_EQ(bufs[0].base, reinterpret_cast<const char*>(data.data()));
//       });
//     });
//   };

//   loop->Run();

//   ASSERT_EQ(gotCallback, 1);
// }

// }  // namespace wpi

// // int main() {}