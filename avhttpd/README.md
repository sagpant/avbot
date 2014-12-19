avhttpd
=======

avhttpd is an header only library that help you develop http based server engines

## avhttpd 是什么
avhttpd 是一个 header only 的库，帮助大家编写基于 HTTP 协议的服务器程序。

## avhttpd 不是什么
avhttpd 并不是一个完整的 HTTP 协议库，只是帮助大家编写简单的 HTTP 协议程序，以配合 nginx 这样的前端工作。
利用 nginx 的  proxy_pass 功能，相对 fastcgi 模式要更加灵活。必要的时候还可以让 avhttpd 编写的程序直接面对用户处理访问。

avhttpd 并不打算支持 gzip/chunked 之类的高级功能， 因为这些都应该是由 nginx 这样的反向代理服务器的工作。

avhttpd 支持 UNIX Socket， 这也得到了 nginx 的支持。

avhttpd 并不支持 SSL , 需要 SSL 功能的，请使用 nginx 并配置好 proxy_pass

## avhttpd 设计

avhttpd 不是一个面向对象的库， 只是一组 composited-handler , 并以 stack-less 协程技术编写而成。

使用 avhttpd 并不会绑架你的库。因为这些功能并不需要你继承任何对象，并且也无需使用 avhttpd 提供的任何对象。

事实上 avhttpd 并不提供任何对象，他只是一组 asio 复合回调函数。

## API 简略

// 读取 HTTP 头，使用 async_read_some 继续读取body
template<class Stream,class MutableBuffer, Handler>
avhttpd::session::async_read_request(Stream & stream, const MutableBuffer &buffer, Handler handler);

// 写入 HTTP 头，使用 async_write_some 继续写入body
template<class Stream,class ConstBuffer, Handler>
avhttpd::session::async_write_response(Stream & stream, const ConstBuffer &buffer, Handler handler);

// 派发 request, 依据已经注册好的 request_uri 和处理函数.
avhttpd::httpd::async_dispatch_request();


