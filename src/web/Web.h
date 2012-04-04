#ifndef DEF_OAKFOAM_WEB_H
#define DEF_OAKFOAM_WEB_H

#include "config.h"
#ifdef HAVE_WEB

#include <string>
#define BOOST_THREAD_USE_LIB
#define BOOST_FILESYSTEM_USE_LIB
#define BOOST_SYSTEM_USE_LIB
#define BOOST_ASIO_USE_LIB
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
//from "../engine/Engine.h":
class Engine;

#define HTTP_VERSION "HTTP/1.1"
#define DOC_ROOT "www"

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
    
    boost::mutex enginemutex;

    void handleConnection(socket_ptr sock);
    void handleRequest(socket_ptr sock, std::string request);
    void handleGet(socket_ptr sock, std::string request);

    void respondBasic(socket_ptr sock, std::string status, std::string type, std::string body);
    void respondBasic(socket_ptr sock, std::string status, std::string body);
    void respondBasic(socket_ptr sock, std::string status);
    void respondStatic(socket_ptr sock, std::string uri);
    void respondGtp(socket_ptr sock, std::string uri);
    void respondJson(socket_ptr sock, std::string uri);

    std::string getMimeType(std::string extension) const;
};

#endif
#endif

