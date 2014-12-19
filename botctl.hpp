#pragma once

#include "libavbot/avbot.hpp"

// 命令控制, 所有的协议都能享受的命令控制在这里实现.
// msg_sender 是一个函数, on_command 用它发送消息.
void on_bot_command(channel_identifier cid, avbotmsg avmessage, send_avchannel_message_t sendmsg, boost::asio::yield_context yield_context, avbot& mybot, avchannel& channel);

void set_do_vc(std::function<void(std::string)>);
void set_do_vc();
