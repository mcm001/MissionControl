#include <vector>

#include <wpi/span.h>

#include "gtest/gtest.h"
#include "wpinet/uv/Loop.h"
#include "wpinet/uv/Pipe.h"
#include "wpinet/uv/Timer.h"

#include <wpinet/WebSocket.h>  // NOLINT(build/include_order)
#include <wpi/StringExtras.h>
#include "wpinet/HttpParser.h"

#include <iostream>


namespace wpi {

namespace uv = wpi::uv;

class WebSocketTest : public ::testing::Test {
 public:
  static const char* pipeName;

  static void SetUpTestCase();

  WebSocketTest() {
    loop = uv::Loop::Create();
    clientPipe = uv::Pipe::Create(loop);
    serverPipe = uv::Pipe::Create(loop);

    serverPipe->Bind(pipeName);

#if 0
    auto debugTimer = uv::Timer::Create(loop);
    debugTimer->timeout.connect([this] {
      std::printf("Active handles:\n");
      uv_print_active_handles(loop->GetRaw(), stdout);
    });
    debugTimer->Start(uv::Timer::Time{100}, uv::Timer::Time{100});
    debugTimer->Unreference();
#endif

    auto failTimer = uv::Timer::Create(loop);
    failTimer->timeout.connect([this] {
      loop->Stop();
      FAIL() << "loop failed to terminate";
    });
    failTimer->Start(uv::Timer::Time{1000});
    failTimer->Unreference();
  }

  ~WebSocketTest() override {
    Finish();
  }

  void Finish() {
    loop->Walk([](uv::Handle& it) { it.Close(); });
  }

  static std::vector<uint8_t> BuildHeader(uint8_t opcode, bool fin,
                                          bool masking, uint64_t len);
  static std::vector<uint8_t> BuildMessage(uint8_t opcode, bool fin,
                                           bool masking,
                                           span<const uint8_t> data);
  static void AdjustMasking(span<uint8_t> message);
  static const uint8_t testMask[4];

  std::shared_ptr<uv::Loop> loop;
  std::shared_ptr<uv::Pipe> clientPipe;
  std::shared_ptr<uv::Pipe> serverPipe;
};



// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.



#ifdef _WIN32
const char* WebSocketTest::pipeName = "\\\\.\\pipe\\websocket-unit-test";
#else
const char* WebSocketTest::pipeName = "/tmp/websocket-unit-test";
#endif
const uint8_t WebSocketTest::testMask[4] = {0x11, 0x22, 0x33, 0x44};

void WebSocketTest::SetUpTestCase() {
#ifndef _WIN32
  unlink(pipeName);
#endif
}

std::vector<uint8_t> WebSocketTest::BuildHeader(uint8_t opcode, bool fin,
                                                bool masking, uint64_t len) {
  std::vector<uint8_t> data;
  data.push_back(opcode | (fin ? 0x80u : 0x00u));
  if (len < 126) {
    data.push_back(len | (masking ? 0x80 : 0x00u));
  } else if (len < 65536) {
    data.push_back(126u | (masking ? 0x80 : 0x00u));
    data.push_back(len >> 8);
    data.push_back(len & 0xff);
  } else {
    data.push_back(127u | (masking ? 0x80u : 0x00u));
    for (int i = 56; i >= 0; i -= 8) {
      data.push_back((len >> i) & 0xff);
    }
  }
  if (masking) {
    data.insert(data.end(), &testMask[0], &testMask[4]);
  }
  return data;
}

std::vector<uint8_t> WebSocketTest::BuildMessage(uint8_t opcode, bool fin,
                                                 bool masking,
                                                 span<const uint8_t> data) {
  auto finalData = BuildHeader(opcode, fin, masking, data.size());
  size_t headerSize = finalData.size();
  finalData.insert(finalData.end(), data.begin(), data.end());
  if (masking) {
    uint8_t mask[4] = {finalData[headerSize - 4], finalData[headerSize - 3],
                       finalData[headerSize - 2], finalData[headerSize - 1]};
    int n = 0;
    for (size_t i = headerSize, end = finalData.size(); i < end; ++i) {
      finalData[i] ^= mask[n++];
      if (n >= 4) {
        n = 0;
      }
    }
  }
  return finalData;
}

// If the message is masked, changes the mask to match the mask set by
// BuildHeader() by unmasking and remasking.
void WebSocketTest::AdjustMasking(span<uint8_t> message) {
  if (message.size() < 2) {
    return;
  }
  if ((message[1] & 0x80) == 0) {
    return;  // not masked
  }
  size_t maskPos;
  uint8_t len = message[1] & 0x7f;
  if (len == 126) {
    maskPos = 4;
  } else if (len == 127) {
    maskPos = 10;
  } else {
    maskPos = 2;
  }
  uint8_t mask[4] = {message[maskPos], message[maskPos + 1],
                     message[maskPos + 2], message[maskPos + 3]};
  message[maskPos] = testMask[0];
  message[maskPos + 1] = testMask[1];
  message[maskPos + 2] = testMask[2];
  message[maskPos + 3] = testMask[3];
  int n = 0;
  for (auto& ch : message.subspan(maskPos + 4)) {
    ch ^= mask[n] ^ testMask[n];
    if (++n >= 4) {
      n = 0;
    }
  }
}

class WebSocketServerTest : public WebSocketTest {
 public:
  WebSocketServerTest() {
    resp.headersComplete.connect([this](bool) { headersDone = true; });

    serverPipe->Listen([this]() {
      auto conn = serverPipe->Accept();
      ws = WebSocket::CreateServer(*conn, "foo", "13");
      if (setupWebSocket) {
        setupWebSocket();
      }
    });
    clientPipe->Connect(pipeName, [this]() {
      clientPipe->StartRead();
      clientPipe->data.connect([this](uv::Buffer& buf, size_t size) {
        std::string_view data{buf.base, size};
        if (!headersDone) {
          data = resp.Execute(data);
          if (resp.HasError()) {
            Finish();
          }
          ASSERT_EQ(resp.GetError(), HPE_OK)
              << http_errno_name(resp.GetError());
          if (data.empty()) {
            return;
          }
        }
        for (auto i : data) {
          std::cout << data;
        }
        std::cout << std::endl;
        wireData.insert(wireData.end(), data.begin(), data.end());
        if (handleData) {
          handleData(data);
        }
      });
      clientPipe->end.connect([this]() { Finish(); });
    });
  }

  std::function<void()> setupWebSocket;
  std::function<void(std::string_view)> handleData;
  std::vector<uint8_t> wireData;
  std::shared_ptr<WebSocket> ws;
  HttpParser resp{HttpParser::kResponse};
  bool headersDone = false;
};

class WebSocketServerDataTest : public WebSocketServerTest,
                                public ::testing::WithParamInterface<size_t> {};

INSTANTIATE_TEST_SUITE_P(WebSocketServerDataTests, WebSocketServerDataTest,
                         ::testing::Values(1));

TEST_P(WebSocketServerDataTest, SendText) {
  int gotCallback = 0;
  std::string str = "Hello!\0";
  std::vector<uint8_t> data = {str.begin(), str.end()};
  setupWebSocket = [&] {
    ws->open.connect([&](std::string_view) {
      ws->SendText({{data}}, [&](auto bufs, uv::Error) {
        ++gotCallback;
        ws->Terminate();
        ASSERT_FALSE(bufs.empty());
        ASSERT_EQ(bufs[0].base, reinterpret_cast<const char*>(data.data()));
      });
    });
  };

  loop->Run();

  auto expectData = BuildMessage(0x01, true, false, data);
  ASSERT_EQ(wireData, expectData);
  ASSERT_EQ(gotCallback, 1);
}

}  // namespace wpi

// int main() {}