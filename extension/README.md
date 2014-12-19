avbot-extension avbot 扩展

这个目录包括了一些扩展功能，根据需要，部分扩展功能可能会被合并入主程序。

目前实现的扩展有

* joke - 聊天的时候发个笑话活跃一下群里的气氛 (可关闭)

* urlpreview - 解析聊天中发送的 URL , 自动将 URL 的页面下载然后将标题贴到频道里。

* bulletin - 电子公告, 每天自动定时的向群里发送设定的群公告 (可关闭)

* static\_content - 对符合某些规则的消息，做出回应

	需要工作目录下的配置文件 static.xml
	
	```
	<static>
		<item>
			<keyword>hyq|mixer</keyword>
			<messages>
				<message>哈哈哈哈，谁在叫我</message>
			</messages>
		</item>
	</static>
	```
	
* python script - python支持
	
	需要在编译时开启，依赖python和boost.python模块
	
	```
	# coding: UTF-8
	import json
	decoder = json.JSONDecoder()
	
	# MessageHandler.send_message 将在cpp里面设置
	class MessageHandler:
	
		def on_message(self, str):
			msg = decoder.decode(str)
			if msg["who"]["name"] == "hyq":
				self.send_message("hyq 你好")
	
	print "python module loaded"
	
	```
