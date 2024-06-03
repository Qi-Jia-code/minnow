#include <cstdlib>
#include <iostream>
#include <socket.hh>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  TCPSocket sock;
  Address addr( host, "http" ); // new一个address类，传入域名和协议
  // addr.from_ipv4_numeric(AF_INET);// 表示使用 IPv4 地址		可选参数
  // string ip_address = "104.196.238.229";
  // uint16_t port_number = 80;
  // pair<std::string, uint16_t> ip_port_pair(ip_address, port_number);

  sock.connect( addr ); // tcp进行三次握手连接

  sock.write( "GET " + path + " HTTP/1.1\r\n" ); // 在tcp三次链接的基础之上，编写http协议的请求
  sock.write( "HOST: " + host + "\r\n" );
  sock.write( "Connection: close\r\n" );
  sock.write( "\r\n" );
  string buffer;

  while ( !sock.eof() ) { // TCPSocket类中包含的eof函数，用于判断stream流是否读取完毕
    sock.read( buffer );  // 将收到的报文读取出来，放入buffer中
    cout << buffer;
  }
  sock.close(); // tcp四次挥手
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}