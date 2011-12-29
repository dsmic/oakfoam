#ifndef DEF_OAKFOAM_WEB_H
#define DEF_OAKFOAM_WEB_H

#include "config.h"
#ifdef HAVE_WEB

#include <string>
#include <boost/asio.hpp>
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
//from "../engine/Engine.h":
class Engine;

#define HTTP_VERSION "HTTP/1.1"

/** Web interface.
 * Alternative interface for engine.
 * This class makes extensive use of the Boost examples.
 */
class Web
{
  public:
    /** Create a new Web object on a port. */
    Web(Engine *eng, std::string a, int p);

    /** Return the engine. */
    Engine *getEngine() const { return engine; };
    /** Return the port. */
    int getPort() const { return port; };

    /** Run the web interface handler. */
    void run();
    
  private:
    Engine *engine;
    std::string addr;
    int port;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor;

    void handleConnection(socket_ptr sock);
    void handleRequest(socket_ptr sock, std::string request);
    void handleGet(socket_ptr sock, std::string request);

    void respondBasic(socket_ptr sock, std::string status, std::string body);
    void respondBasic(socket_ptr sock, std::string status);
    void respondStatic(socket_ptr sock, std::string uri);

    std::string getMimeType(std::string extension);
};

#endif
#endif

