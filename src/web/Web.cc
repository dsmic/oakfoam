#include "Web.h"
#ifdef HAVE_WEB

#include <cstdio>
#include <string>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include "../engine/Engine.h"

using boost::asio::ip::tcp;
typedef boost::shared_ptr<tcp::socket> socket_ptr;

Web::Web(Engine *eng, int p)
{
  engine=eng;
  port=p;
}

void Web::run()
{
  fprintf(stderr,"running web interface on port %d...\n",port);

  boost::asio::io_service io_service;
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
  
  while(true)
  {
    socket_ptr sock(new tcp::socket(io_service));
    acceptor.accept(*sock);
    while(true)
    {
      char data[256];

      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.
      data[length]='\0';

      fprintf(stderr,"recv: ^%s$\n",data);
      if (!engine->getGtpEngine()->executeCommand(data))
        return;
      //boost::asio::write(*sock, boost::asio::buffer(data, length));
    }
  }
}

#endif

