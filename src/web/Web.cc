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
  fprintf(stderr,"running web interface on http://%s:%d\n",addr.c_str(),port);

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
  if (uri.find(".gtpcmd")!=std::string::npos)
    this->respondGtp(sock,uri);
  else if (uri.find(".jsoncmd")!=std::string::npos)
    this->respondJson(sock,uri);
  else
    this->respondStatic(sock,uri);
}

void Web::respondBasic(socket_ptr sock, std::string status, std::string type, std::string body)
{
  std::ostringstream ss;
  ss<<HTTP_VERSION<<" "<<status<<"\r\n";
  ss<<"Content-Type: "<<type<<"\r\n";
  ss<<"Content-Length: "<<body.length()<<"\r\n";
  ss<<"\r\n"<<body;
  boost::asio::write(*sock, boost::asio::buffer(ss.str()));
}

void Web::respondBasic(socket_ptr sock, std::string status, std::string body)
{
  this->respondBasic(sock,status,"text/html",body);
}

void Web::respondBasic(socket_ptr sock, std::string status)
{
  std::string body="<html><head>\n<title>"+status+"</title>\n</head><body>\n<h1>"+status+"</h1>\n</body></html>";
  this->respondBasic(sock,status,body);
}

void Web::respondStatic(socket_ptr sock, std::string uri)
{
  namespace fs = boost::filesystem;

  size_t fieldpos=uri.find("?");
  std::string path=DOC_ROOT+uri.substr(0,fieldpos);
  // below piece doesn't work in windows build as-is
  //if (fs::is_directory(fs::path(path.c_str(),fs::native)) && path[path.length()-1]!='/')
  //  path+="/";
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

void Web::respondGtp(socket_ptr sock, std::string uri)
{
  size_t startpos=uri.rfind("/");
  size_t endpos=uri.rfind(".gtpcmd");
  if (startpos==std::string::npos)
    startpos=0;
  std::string cmd=uri.substr(startpos+1,endpos-startpos-1);

  size_t fieldpos=uri.find("?");
  if (fieldpos!=std::string::npos)
  {
    std::string args=uri.substr(fieldpos+1);
    cmd+=" ";
    for (unsigned int i=0;i<args.length();i++)
    {
      if (args[i]=='&')
        cmd+=" ";
      else
        cmd+=args[i];
    }
  }

  if (cmd.substr(0,4)=="stop")
  {
    engine->stopThinking();
    this->respondBasic(sock,"200 Ok","text/plain");
  }
  else
  {
    boost::mutex::scoped_lock lock(enginemutex);
    fprintf(stderr,"GTP cmd: '%s'\n",cmd.c_str());

    std::string output="";
    engine->getGtpEngine()->getOutput()->setRedirectStrings(&output,NULL);
    engine->getGtpEngine()->executeCommand(cmd); // TODO: should be making use of a mutex here
    engine->getGtpEngine()->getOutput()->setRedirectStrings(NULL,NULL);

    this->respondBasic(sock,"200 Ok","text/plain",output);
  }
}

void Web::respondJson(socket_ptr sock, std::string uri)
{
  size_t startpos=uri.rfind("/");
  size_t endpos=uri.rfind(".jsoncmd");
  if (startpos==std::string::npos)
    startpos=0;
  std::string cmd=uri.substr(startpos+1,endpos-startpos-1);

  size_t fieldpos=uri.find("?");
  std::vector<std::string> args;
  if (fieldpos!=std::string::npos)
  {
    std::string argstring=uri.substr(fieldpos+1);
    std::string arg="";
    for (unsigned int i=0;i<argstring.length();i++)
    {
      if (argstring[i]=='&')
      {
        args.push_back(arg);
        arg="";
      }
      else
        arg+=argstring[i];
    }
    args.push_back(arg);
  }

  boost::mutex::scoped_lock lock(enginemutex);
  fprintf(stderr,"JSON cmd: %s\n",cmd.c_str());

  std::ostringstream out;
  out<<"{\n";
  if (cmd=="engine_info")
  {
    out<<"\"name\": \""<<PACKAGE_NAME<<"\",\n";
    out<<"\"version\": \""<<PACKAGE_VERSION<<"\"\n"; // no 'trailing' comma
  }
  else if (cmd=="board_info")
  {
    int size=engine->getBoardSize();
    out<<"\"size\": "<<size<<",\n";
    out<<"\"komi\": "<<engine->getKomi()<<",\n";
    out<<"\"moves\": "<<engine->getCurrentBoard()->getMovesMade()<<",\n";
    out<<"\"last_move\": \""<<engine->getCurrentBoard()->getLastMove().toString(size)<<"\",\n";
    out<<"\"next_color\": \""<<Go::colorToChar(engine->getCurrentBoard()->nextToMove())<<"\",\n";
    out<<"\"simple_ko\": \""<<Go::Position::pos2string(engine->getCurrentBoard()->getSimpleKo(),size)<<"\",\n";
    out<<"\"passes\": "<<engine->getCurrentBoard()->getPassesPlayed()<<",\n";
    out<<"\"threads\": "<<engine->getParams()->thread_count<<",\n";
    out<<"\"playouts\": "<<engine->getParams()->playouts_per_move<<",\n";
    out<<"\"time\": "<<engine->getParams()->time_move_max<<",\n";
    out<<"\"black_captures\": "<<engine->getCurrentBoard()->getStoneCapturesOf(Go::BLACK)<<",\n";
    out<<"\"white_captures\": "<<engine->getCurrentBoard()->getStoneCapturesOf(Go::WHITE)<<"\n"; // no 'trailing' comma
  }
  else if (cmd=="board_pos")
  {
    int size=engine->getBoardSize();
    Go::Board *board=engine->getCurrentBoard();
    for (int x=0;x<size;x++)
    {
      for (int y=0;y<size;y++)
      {
        int pos=Go::Position::xy2pos(x,y,size);
        out<<"\""<<Go::Position::pos2string(pos,size)<<"\": \""<<Go::colorToChar(board->getColor(pos))<<"\"";
        if (x<(size-1)||y<(size-1)) // no 'trailing' comma
          out<<",\n";
        else
          out<<"\n";
      }
    }
  }
  else if (cmd=="board_scored")
  {
    int size=engine->getBoardSize();
    Go::Board *board=engine->getCurrentBoard();
    // assumed scoring is done
    for (int x=0;x<size;x++)
    {
      for (int y=0;y<size;y++)
      {
        int pos=Go::Position::xy2pos(x,y,size);
        out<<"\""<<Go::Position::pos2string(pos,size)<<"\": \""<<Go::colorToChar(board->getScoredOwner(pos))<<"\"";
        if (x<(size-1)||y<(size-1)) // no 'trailing' comma
          out<<",\n";
        else
          out<<"\n";
      }
    }
  }
  else
  {
    this->respondBasic(sock,"404 Not Found");
    return;
  }
  //engine->getGtpEngine()->getOutput()->setRedirectStrings(&output,NULL);
  //engine->getGtpEngine()->executeCommand(cmd);
  //engine->getGtpEngine()->getOutput()->setRedirectStrings(NULL,NULL);
  out<<"}\n";

  this->respondBasic(sock,"200 Ok","application/json",out.str());
}

std::string Web::getMimeType(std::string ext) const
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
  else if (ext=="json")
    return "application/json";
  else
    return "text/plain";
}

#endif

