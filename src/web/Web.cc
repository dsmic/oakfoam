#include "Web.h"
#ifdef HAVE_WEB

#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include "../engine/Engine.h"

Web::Web(Engine *eng, std::string a, int p)
  : io_service(),
    acceptor(io_service)
{
  engine=eng;
  addr=a;
  port=p;

  boost::asio::ip::tcp::resolver resolver(io_service);
  std::ostringstream ss;
  ss<<port;
  boost::asio::ip::tcp::resolver::query query(addr,ss.str());
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  acceptor.open(endpoint.protocol());
  acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor.bind(endpoint);
  acceptor.listen();
}

void Web::run()
{
  fprintf(stderr,"running web interface on %s:%d...\n",addr.c_str(),port);

  while(true)
  {
    socket_ptr sock(new boost::asio::ip::tcp::socket(io_service));
    acceptor.accept(*sock);
    boost::thread thrd(boost::bind(&Web::handleConnection,this,sock));
  }
}


void Web::handleConnection(socket_ptr sock)
{
  //boost::asio::write(*sock, boost::asio::buffer("meh!\n\n"));
  char buffer[1024];
  std::string request="";

  while (true)
  {
    boost::system::error_code error;
    size_t length = sock->read_some(boost::asio::buffer(buffer), error);
    if (error == boost::asio::error::eof)
      return; // connection closed cleanly by peer
    else if (error)
      throw boost::system::system_error(error); // some other error
    buffer[length]='\0';
    request+=buffer;
    
    if (request.length()>=4) // look for end of request
    {
      size_t pos=request.find("\r\n\r\n");
      if (pos!=std::string::npos)
      {
        request=request.substr(0,pos); // strip out anything after headers
        break;
      }
    }

    //engine->getGtpEngine()->executeCommand(buffer);
  }

  //fprintf(stderr,"recv: ^%s$\n",request.c_str());
  this->handleRequest(sock,request);
}

void Web::handleRequest(socket_ptr sock, std::string request)
{
  std::istringstream iss(request);

  std::string method;
  if (!getline(iss,method,' '))
    return; // missing method

  if (method=="GET")
    this->handleGet(sock,request);
  else
    this->respondBasic(sock,"501 Not Implemented");
}

void Web::handleGet(socket_ptr sock, std::string request)
{
  std::istringstream iss(request);

  std::string method;
  if (!getline(iss,method,' ') || method!="GET")
  {
    this->respondBasic(sock,"400 Bad Request");
    return;
  }

  std::string uri;
  if (!getline(iss,uri,' '))
  {
    this->respondBasic(sock,"400 Bad Request");
    return;
  }

  std::string httpver;
  if (!getline(iss,httpver,'\r'))
  {
    this->respondBasic(sock,"400 Bad Request");
    return;
  }
  else if (httpver!=HTTP_VERSION)
  {
    this->respondBasic(sock,"505 HTTP Version Not Supported");
    return;
  }
  getline(iss,httpver,'\n');

  fprintf(stderr,"GET %s\n",uri.c_str());
  this->respondStatic(sock,uri);
}

void Web::respondBasic(socket_ptr sock, std::string status, std::string body)
{
  std::ostringstream ss;
  ss<<HTTP_VERSION<<" "<<status<<"\r\n";
  ss<<"Content-Type: text/html\r\n";
  ss<<"Content-Length: "<<body.length()<<"\r\n";
  ss<<"\r\n"<<body;
  boost::asio::write(*sock, boost::asio::buffer(ss.str()));
}

void Web::respondBasic(socket_ptr sock, std::string status)
{
  std::string body="<html><head>\n<title>"+status+"</title>\n</head><body>\n<h1>"+status+"</h1>\n</body></html>";
  this->respondBasic(sock,status,body);
}

void Web::respondStatic(socket_ptr sock, std::string uri)
{
  namespace fs = boost::filesystem;

  std::string path="."+uri;
  if (fs::is_directory(fs::path(path.c_str(),fs::native)) && path[path.length()-1]!='/')
    path+="/";
  if (path[path.length()-1]=='/')
    path+="index.html";

  std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
  if (!is)
  {
    this->respondBasic(sock,"404 Not Found");
    return;
  }

  size_t last_slash_pos=path.find_last_of("/");
  size_t last_dot_pos=path.find_last_of(".");
  std::string extension;
  if (last_dot_pos!=std::string::npos && last_dot_pos>last_slash_pos)
  {
    extension=path.substr(last_dot_pos+1);
  }

  std::ostringstream ss;
  ss<<HTTP_VERSION<<" 200 Ok\r\n";
  std::string content="";
  char buf[512];
  while (is.read(buf, sizeof(buf)).gcount() > 0)
    content.append(buf, is.gcount());
  ss<<"Content-Type: "<<this->getMimeType(extension)<<"\r\n";
  ss<<"Content-Length: "<<content.length()<<"\r\n";
  ss<<"\r\n"<<content;
  boost::asio::write(*sock, boost::asio::buffer(ss.str()));
}

std::string Web::getMimeType(std::string ext)
{
  if (ext=="htm"||ext=="html")
    return "text/html";
  else if (ext=="png")
    return "image/png";
  else if (ext=="jpg"||ext=="jpeg")
    return "image/jpeg";
  else if (ext=="gif")
    return "image/gif";
  else if (ext=="js")
    return "application/javascript";
  else if (ext=="css")
    return "text/css";
  else
    return "text/plain";
}

#endif

