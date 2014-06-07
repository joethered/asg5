// $Id: cix-client.cpp,v 1.4 2014-05-30 12:47:58-07 - - $

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "cix_protocol.h"
#include "logstream.h"
#include "signal_action.h"
#include "sockets.h"

logstream log (cout);

unordered_map<string,cix_command> command_map {
   {"exit", CIX_EXIT},
   {"get",  CIX_GET },
   {"help", CIX_HELP},
   {"ls"  , CIX_LS  },
   {"put",  CIX_PUT },
   {"rm",   CIX_RM  },

};

void cix_help() {
   static vector<string> help = {
      "exit         - Exit the program.  Equivalent to EOF.",
      "get filename - Copy remote file to local host.",
      "help         - Print help summary.",
      "ls           - List names of files on remote server.",
      "put filename - Copy local file to remote host.",
      "rm filename  - Remove file from remote server.",
   };
   for (const auto& line: help) cout << line << endl;
}

void cix_ls (client_socket& server) {
   cix_header header;
   header.cix_command = CIX_LS;
   log << "sending header " << header << endl;
   send_packet (server, &header, sizeof header);
   recv_packet (server, &header, sizeof header);
   log << "received header " << header << endl;
   if (header.cix_command != CIX_LSOUT) {
      log << "sent CIX_LS, server did not return CIX_LSOUT" << endl;
      log << "server returned " << header << endl;
   }else {
      char buffer[header.cix_nbytes + 1];
      recv_packet (server, buffer, header.cix_nbytes);
      log << "received " << header.cix_nbytes << " bytes" << endl;
      buffer[header.cix_nbytes] = '\0';
      cout << buffer;
   }
}

bool is_port(string& port)
{
    if(port.size()!= 15)
    {
        return false;
    }

    for(size_t i=1; i<=port.size(); ++i)
    {
        //example: 128.114.108.152
        if(i%4==0)
        {
            if(port[i-1]!='.')
                return false;
        }
        else
        {
            if(port[i-1]<48 || port[i-1]>57)
                return false;
        }
    }

    return true;
}

void cix_get(){}
void cix_rm(){}
void cix_put() {}


void usage() {
   cerr << "Usage: " << log.execname() << " [host] [port]" << endl;
   throw cix_exit();
}

bool signal_handler_throw_cix_exit {false};
void signal_handler (int signal) {
   log << "signal_handler: caught " << strsignal (signal) << endl;
   switch (signal) {
      case SIGINT: case SIGTERM: signal_handler_throw_cix_exit = true;
      default: break;
   }
}

int main (int argc, char** argv) {
   log.execname (basename (argv[0]));
   log << "starting" << endl;
   vector<string> args (&argv[1], &argv[argc]);
   //signal_action (SIGINT, signal_handler);
   //signal_action (SIGTERM, signal_handler);
   if (args.size() > 2) usage();
   if (args.size() == 2){
      string host = get_cix_server_host (args, 0);
      in_port_t port = get_cix_server_port (args, 1);
   }else if (args.size() == 1){
      if ( is_port() ){
         string host = get_cix_server_host (args, 0);
         in_port_t port = CIX_SERVER_PORT;
      }else{
         string host = CIX_SERVER_HOST;
         in_port_t port = get_cix_server_port (args, 0);
      }
   }
   log << to_string (hostinfo()) << endl;
   try {
      log << "connecting to " << host << " port " << port << endl;
      client_socket server (host, port);
      log << "connected to " << to_string (server) << endl;
      for (;;) {
         string line;
         getline (cin, line);
         if (cin.eof()) throw cix_exit();
         log << "command " << line << endl;
         const auto& itor = command_map.find (line);
         cix_command cmd = itor == command_map.end()
                         ? CIX_ERROR : itor->second;
         switch (cmd) {
            case CIX_EXIT:
               throw cix_exit();
               break;
            case CIX_HELP:
               cix_help();
               break;
            case CIX_LS:
               cix_ls (server);
               break;
            default:
               log << line << ": invalid command" << endl;
               break;
         }
      }
   }catch (socket_error& error) {
      log << error.what() << endl;
   }catch (cix_exit& error) {
      log << "caught cix_exit" << endl;
   }
   log << "finishing" << endl;
   return 0;
}

