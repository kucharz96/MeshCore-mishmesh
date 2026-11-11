#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/SensorManager.h>
#include <helpers/MultiSerialInterface.h>
#include <Arduino.h>

#ifdef PIN_BUZZER
  #include <helpers/ui/buzzer.h>
#endif

#include "NodePrefs.h"
#include "MyMesh.h"

enum class UIEventType {
    none,
    contactMessage,
    channelMessage,
    roomMessage,
    newContactMessage,
    ack
};

class AbstractUITask : public MyMesh::Listener {
protected:
  mesh::MainBoard* _board;
  MultiSerialInterface* _interfaceManager;

  AbstractUITask(mesh::MainBoard* board, MultiSerialInterface* interfaceManager) : _board(board), _interfaceManager(interfaceManager) {
  }

public:
  bool hasConnection() const { return _interfaceManager->isConnected(); }
  uint16_t getBattMilliVolts() const { return _board->getBattMilliVolts(); }
  bool isBluetoothEnabled() const { return _interfaceManager->isBluetoothEnabled(); }
  void enableBluetooth() { _interfaceManager->enableBluetooth(); }
  void disableBluetooth() { _interfaceManager->disableBluetooth(); }

  // Compatibility layer: retain MishMesh's existing UI API while using MeshCore 1.18 listeners.
  virtual void msgRead(int msgcount) = 0;
  virtual void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) = 0;

  void onMessageRecv(mesh::Packet* pkt, const ContactInfo& from, uint8_t txt_type,
                     uint32_t sender_timestamp, const char* text) override {
    (void)sender_timestamp;
    if (txt_type != TXT_TYPE_PLAIN && txt_type != TXT_TYPE_SIGNED_PLAIN) return;
    newMsg(pkt ? pkt->path_len : 0, from.name, text, 0);
    if (!hasConnection()) notify(UIEventType::contactMessage);
  }
  void onChannelMessageRecv(mesh::Packet* pkt, ChannelDetails& channel, const char* text) override {
    newMsg(pkt ? pkt->path_len : 0, channel.name, text, 0);
    if (!hasConnection()) notify(UIEventType::channelMessage);
  }
  void onQueueSizeChanged(int count) override { msgRead(count); }
  void onACKRecv(uint32_t crc) override { (void)crc; notify(UIEventType::ack); }
  void onDiscoveredContact(ContactInfo &contact, bool is_new, uint8_t path_len,
                           const uint8_t* path) override {
    (void)contact; (void)path_len; (void)path;
    if (!is_new) notify(UIEventType::newContactMessage);
  }
  virtual void notify(UIEventType t = UIEventType::none) = 0;
  virtual void loop() = 0;
  // [mishmesh] Result of a room/repeater login initiated on-device (mishmesh).
  // Default no-op keeps the companion/legacy UIs unaffected. `perms` are the
  // server's ACL permission bits (0 = guest/read-only).
  virtual void onRoomLogin(const uint8_t* pubkey, bool success, bool is_admin, uint8_t perms) {
    (void)pubkey; (void)success; (void)is_admin; (void)perms;
  }
  // [/mishmesh]
};
