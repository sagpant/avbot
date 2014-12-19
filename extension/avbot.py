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
