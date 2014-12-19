
--  使用 send_channel_message 将消息发回频道.
--  send_channel_message('test')


-- 每当频道有消息发生, 就调用这个函数.
-- 注意, 不要在这里阻塞, 会导致整个 avbot 都被卡住
-- 因为 avbot 是单线程程序.
--[=[examplejson=[[
{
    "protocol": "qq",
    "channel": "8734129",
    "room":
    {
        "code": "3964816318",
        "groupnumber": "8734129",
        "name": "Ballache"
    },
    "op": "0",
    "who":
    {
        "code": "3890512126",
        "nick": "Aiephen",
        "name": "Aiephen",
        "qqnumber": "24237081",
        "card": "Bearkid Broodmother"
    },
    "preamble": "qq(Bearkid Broodmother) :",
    "message":
    {
        "text": "2\u00E4\u00BB\u00A3\u00E5\u008A\u009F\u00E8\u0083\u00BD\u00E6\u009B\u00B4\u00E5\u00BC\u00BA\u00E4\u00BA\u0086\u00EF\u00BC\u008C\u00E4\u00B8\u008D\u00E8\u00BF\u0087\u00E5\u009B\u00A0\u00E4\u00B8\u00BA\u00E7\u009B\u00B4\u00E6\u008E\u00A5\u00E6\u0098\u00AF1\u00E4\u00BB\u00A3\u00E7\u00A7\u00BB\u00E6\u00A4\u008D\u00E8\u00BF\u0087\u00E5\u008E\u00BB\u00E7\u009A\u0084\u00EF\u00BC\u008C\u00E9\u0087\u008C\u00E9\u009D\u00A2\u00E6\u009C\u0089\u00E4\u00B8\u008D\u00E5\u00B0\u0091\u00E6\u0097\u00A7\u00E6\u0097\u00B6\u00E4\u00BB\u00A3\u00E9\u0081\u0097\u00E7\u0095\u0099\u00E4\u00BA\u00A7\u00E5\u0093\u0081 "
    }
}
]]
]=]
package.path="lua_libraries/?.lua;luascript/?.lua;?.lua"
package.cpath="lua_libraries/?.so;lua_libraries/?.dll;"..package.cpath
require('json')

--print(msg_table.who.nick)
--print(msg_table.message.text)

main_help_table={
"qqdicebot,第4版。开发者：BX。\n免责声明：本机器人提供的内容要么来自网上，要么来自一些会打字的大猩猩。对这些内容的真实可信性开发者概不负责。\n帮助链接：http://trow.cc/forum/index.php?showtopic=19753"
}
--loadfile("luascript\\random.lua")()
--loadfile("luascript\\bet.lua")()
--2月5日改用require
require"dnddice"
require"woddice"
require"roll"
--require"getlink"
--require"dictionary"
--require"wiki"
require"knight"
require"godmachine"
require"zhan"
require"fate"
require"fruit"
require"help"
require"dnddice_detail"
require"tishen"
--require"escape"
require"rememberip"
require"TALK"
require"seventhsea"
require"scp"
--loadfile("luascript\\router.lua")()
--loadfile("luascript\\rss.lua")()

function channel_message(jsonmsg)
	msg_table=json.decode(jsonmsg)
	msg=msg_table.message.text
	msg_time=os.date("%H:%M:%S")
	buddy_name=msg_table.who.card
	buddy_num=msg_table.who.qqnumber
	qun_name=msg_table.room.name
	qun_num=msg_table.room.groupnumber
	say_qun=function (the_msg, qun_num_nouse)
		send_channel_message(the_msg)
	end
	--print(jsonmsg)
		if msg==nil then return end
--	escape(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	--bet(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
--	getlink(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	roll(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	dnddice(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	woddice(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	seventhsea(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
--	dictionary(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
--	wiki(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	knight(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	godmachine(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	zhan(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	fate(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	fruit(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	help(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	dnddice_detail(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	tishen(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	--ronind10(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
--	readip(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
--	setip(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	talk(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	scp(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	--router(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
	--rss(msg,msg_time,buddy_name,buddy_num,qun_name,qun_num)
end

print("main.lua载入成功")
