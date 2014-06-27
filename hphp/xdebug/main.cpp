#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>

#include "hphp/runtime/base/base-includes.h"
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <libxml/uri.h>

using namespace std;

// I stole it form hphp/runtime/server/server-stats.cpp

///////////////////////////////////////////////////////////////////////////////
// writer

class Writer {
public:
  explicit Writer(ostream &out) : m_out(out), m_indent(0) {}
  virtual ~Writer() {}

  virtual void writeFileHeader() = 0;
  virtual void writeFileFooter() = 0;

  // Begins writing an object which is different than a list in JSON.
  virtual void beginObject(const char *name) = 0;

  // Begins writing a list (an ordered set of potentially unnamed children)
  // Defaults to begining an object.
  virtual void beginList(const char *name) {
    beginObject(name);
  }

  // Writes a string value with a given name.
  virtual void writeEntry(const char *name, const std::string &value) = 0;

  // Writes a string value with a given name.
  virtual void writeEntry(const char *name, int64_t value) = 0;

  // Ends the writing of an object.
  virtual void endObject(const char *name) = 0;

  // Ends the writing of a list. Defaults to simply ending an Object.
  virtual void endList(const char *name) {
    endObject(name);
  }

protected:
  ostream &m_out;
  int m_indent;

  virtual void writeIndent() {
    for (int i = 0; i < m_indent; i++) {
      m_out << "  ";
    }
  }
};

class XMLWriter : public Writer {
public:
  explicit XMLWriter(ostream &out) : Writer(out) {}


  virtual void writeFileHeader() {
    m_out << "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
  }

  virtual void writeFileFooter() {}


  /**
   * In XML/HTML there is no distinction between creating a list, and creating
   * an object with keyed attributes. (unlike the JSON format).
   */
  virtual void beginObject(const char *name) {
    writeIndent();
    m_out << '<' << name << ">\n";
    ++m_indent;
  }

  virtual void endObject(const char *name) {
    --m_indent;
    writeIndent();
    m_out << "</" << name << ">\n";
  }

  virtual void writeEntry(const char *name, const string &value) {
    writeIndent();
    m_out << "<entry><key>" << Escape(name) << "</key>";
    m_out << "<value>" << Escape(value.c_str()) << "</value></entry>\n";
  }

  virtual void writeEntry(const char *name, int64_t value) {
    writeIndent();
    m_out << "<entry><key>" << Escape(name) << "</key>";
    m_out << "<value>" << value << "</value></entry>\n";
  }

private:
  static std::string Escape(const char *s) {
    string ret;
    for (const char *p = s; *p; p++) {
      switch (*p) {
      case '<':  ret += "&lt;";  break;
      case '&':  ret += "&amp;"; break;
      default:   ret += *p;      break;
      }
    }
    return ret;
  }
};

// writer
///////////////////////////////////////////////////////////////////////////////

void error(const char *msgBuffer)
{
	perror(msgBuffer);
	exit(-1);
}

class IntelliJCommand
{
	private:

		IntelliJCommand(const char *cmd, const int transactionId) {
	   	  strcpy(_cmd, cmd);
			_transactionId = transactionId;
			// all for now, for this prototype
		}

	public:

		virtual ~IntelliJCommand() {};

		// copy constructor, assignment operators skipped, do deep copy.

		// parse msg form IntelliJ
		static IntelliJCommand *parse(const char *msg) {
			// using plain g++, will use c++11 split or regexp later.
			const char *msgPtr = msg;
			int len = strlen(msg);

			const char *firstSpace = strchr(msgPtr, ' ');
			if (!firstSpace) {
				printf("parse() can't locate first space\n");
				return NULL;
			}

			len = firstSpace - msgPtr;
			if (len >= 255 ) {
				printf("parse() command string too long %d \n", len);
				return NULL;
			}

			char cmd[256];
			strncpy(cmd, msgPtr, len);
			cmd[len] = '\0';
			msgPtr += len;

			msgPtr = strstr(msgPtr, "-i ");
			if (!msgPtr) {
				printf("parse() cant locate transaction id\n");
				return NULL;
			}

			// move past "-i "
			msgPtr += strlen("-i ");

			const char* nextSpace = strchr(msgPtr, ' ');
			// If we can't find next space, the transaction id is at the end of the command string.
			len = (nextSpace)? nextSpace - msgPtr : strlen(msgPtr);

			char transactionIdStr[256];
			if (len >= 255 ) {
				printf("parse() transaction id string too long %d \n", len);
				return NULL;
			}

			strncpy(transactionIdStr, msgPtr, len);
			transactionIdStr[len] = '\0';
			int transactionId = atoi(transactionIdStr);

			return new IntelliJCommand(cmd, transactionId);
		}

		const char *getCmd() const {
	   	   return _cmd;
		}

		const int getTransactionId() const {
			return _transactionId;
		}

	private:

		char _cmd[256];
		int _transactionId;
};

// create a binary byte stream to send over tcp/ip socket containing the input xnl message
// xml header and number of characters are prepended
// number of bytes including null-terminating string is returned:
//
// example: prepare_message("<response xmlns="urn:debugger_protocol_v1" xmlns:xdebug="http://xdebug.org/dbgp/xdebug" command="feature_set" transaction_id="%d" feature="max_children" success="1"></response>", transaction_id)
// will generate
// "219<?xml version="1.0" encoding="iso-8859-1"?>
// <response xmlns="urn:debugger_protocol_v1" xmlns:xdebug="http://xdebug.org/dbgp/xdebug" command="feature_set" transaction_id="<transaction_id>" feature="max_children" success="1"></response>"
// and will return 224
//
size_t  prepare_message(char *msgBuffer /* has to be sufficiently large */, const char *str, int transactionId)
{
	const char *xmlHeader = "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n";
	char sizeStr[255];
	char *bufPtr = msgBuffer;
	char *xmlMsg = new char[strlen(str) + 100];
	if (transactionId >= 0)
		sprintf(xmlMsg, str, transactionId);
	else
		sprintf(xmlMsg, "%s", str);
	size_t msgLen = strlen(xmlHeader) + strlen(xmlMsg);
	sprintf(sizeStr, "%d", (int) msgLen);
	memcpy(bufPtr, sizeStr, strlen(sizeStr));
	// skip 1 char for 0
	bufPtr += (strlen(sizeStr) + 1);
	memcpy(bufPtr, xmlHeader, strlen(xmlHeader)); // no null_terminating string
	bufPtr += strlen(xmlHeader);
	memcpy(bufPtr, xmlMsg, strlen(xmlMsg));
	bufPtr += strlen(xmlMsg) + 1;
	printf("msgLen = %d\nmsg = %s", (int) msgLen, msgBuffer +strlen(sizeStr) + 1);
	return (bufPtr - msgBuffer);
}

int main(int argc, char** argv)
{
        // for some 
        std::ostringstream xmlOstream;
        XMLWriter xmlWriter(xmlOstream);
        xmlWriter.writeFileHeader();
        //cout << xmlOstream.str();

        printf("xdebug starting...\n");
	int sockfd, portno, nwritten;
	struct sockaddr_in serverAddress;
	struct hostent *serverHost;

	int numFeatureResponses = 0;
	const char *featureResponses[10];
	featureResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"feature_set\" transaction_id=\"%d\" feature=\"show_hidden\" success=\"1\"></response>";
	featureResponses[1] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"feature_set\" transaction_id=\"%d\" feature=\"max_depth\" success=\"1\"></response>";
	featureResponses[2] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"feature_set\" transaction_id=\"%d\" feature=\"max_children\" success=\"1\"></response>";

	int numStatusResponses = 0;
	const char *statusResponses[10];
	statusResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"status\" transaction_id=\"%d\" status=\"starting\" reason=\"ok\"></response>";

	int numStepIntoResponses = 0;
	const char *stepIntoResponses[10];
	stepIntoResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"step_into\" transaction_id=\"%d\" status=\"break\" reason=\"ok\"><xdebug:message filename=\"file:///box/www/ochotinun/index.php\" lineno=\"3\"></xdebug:message></response>";

	int numEvalResponses = 0;
	const char *evalResponses[10];
	evalResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"eval\" transaction_id=\"%d\"><property address=\"140737487769840\" type=\"bool\"><![CDATA[0]]></property></response>";
	evalResponses[1] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"eval\" transaction_id=\"%d\"><property address=\"140737487769840\" type=\"bool\"><![CDATA[1]]></property></response>";
	evalResponses[2] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"eval\" transaction_id=\"%d\"><property address=\"140737487769840\" type=\"string\" size=\"24\" encoding=\"base64\"><![CDATA[b2Nob3RpbnVuLmluc2lkZS1ib3gubmV0]]></property></response>";
	evalResponses[3] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"eval\" transaction_id=\"%d\"><property address=\"140737487769840\" type=\"string\" size=\"2\" encoding=\"base64\"><![CDATA[ODA=]]></property></response>";
	evalResponses[4] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"eval\" transaction_id=\"%d\"><property address=\"140737487769840\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Lw==]]></property></response>";

	int numRunResponses = 0;
	const char *runResponses[10];
	runResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"run\" transaction_id=\"%d\" status=\"stopping\" reason=\"ok\"></response>";
	runResponses[1] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"run\" transaction_id=\"%d\" status=\"break\" reason=\"ok\"><xdebug:message filename=\"file:///box/www/ochotinun/index.php\" lineno=\"3\"></xdebug:message></response>";
	runResponses[2] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"run\" transaction_id=\"%d\" status=\"stopping\" reason=\"ok\"></response>";

	int numStackGetResponses = 0;
	const char *stackGetResponses[10];
	stackGetResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"stack_get\" transaction_id=\"%d\"><stack where=\"{main}\" level=\"0\" type=\"file\" filename=\"file:///box/www/ochotinun/index.php\" lineno=\"3\"></stack></response>";
	stackGetResponses[1] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"stack_get\" transaction_id=\"%d\"><stack where=\"{main}\" level=\"0\" type=\"file\" filename=\"file:///box/www/ochotinun/index.php\" lineno=\"3\"></stack></response>";

	int numBreakpointResponses = 0;
	const char *breakpointResponses[10];
	breakpointResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"breakpoint_set\" transaction_id=\"%d\" id=\"203110001\"></response>";

	int numContextNamesResponses = 0;
	const char *contextNamesResponses[10];
	contextNamesResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"context_names\" transaction_id=\"%d\"><context name=\"Locals\" id=\"0\"></context><context name=\"Superglobals\" id=\"1\"></context></response>";

	int numContextGetResponses = 0;
	const char *contextGetResponses[10];
	contextGetResponses[0] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"context_get\" transaction_id=\"%d\" context=\"0\"><property name=\"$num\" fullname=\"$num\" address=\"139672244078352\" type=\"int\"><![CDATA[10]]></property></response>";
	contextGetResponses[1] = "<response xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" command=\"context_get\" transaction_id=\"%d\" context=\"1\"><property name=\"$_COOKIE\" fullname=\"$_COOKIE\" address=\"139672244053272\" type=\"array\" children=\"1\" numchildren=\"3\" page=\"0\" pagesize=\"100\"><property name=\"box_visitor_id\" fullname=\"$_COOKIE[&#39;box_visitor_id&#39;]\" address=\"139672244053992\" type=\"string\" size=\"23\" encoding=\"base64\"><![CDATA[NTMyYjNlZDEzZmMzNzguNjA0OTI3Mjk=]]></property><property name=\"box_locale\" fullname=\"$_COOKIE[&#39;box_locale&#39;]\" address=\"139672244054376\" type=\"string\" size=\"5\" encoding=\"base64\"><![CDATA[ZW5fVVM=]]></property><property name=\"presentation\" fullname=\"$_COOKIE[&#39;presentation&#39;]\" address=\"139672244054744\" type=\"string\" size=\"7\" encoding=\"base64\"><![CDATA[ZGVza3RvcA==]]></property></property><property name=\"$_ENV\" fullname=\"$_ENV\" address=\"139672244055416\" type=\"array\" children=\"1\" numchildren=\"15\" page=\"0\" pagesize=\"100\"><property name=\"TERM\" fullname=\"$_ENV[&#39;TERM&#39;]\" address=\"139672244055584\" type=\"string\" size=\"5\" encoding=\"base64\"><![CDATA[bGludXg=]]></property><property name=\"PATH\" fullname=\"$_ENV[&#39;PATH&#39;]\" address=\"139672244055760\" type=\"string\" size=\"29\" encoding=\"base64\"><![CDATA[L3NiaW46L3Vzci9zYmluOi9iaW46L3Vzci9iaW4=]]></property><property name=\"runlevel\" fullname=\"$_ENV[&#39;runlevel&#39;]\" address=\"139672244055936\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Mw==]]></property><property name=\"RUNLEVEL\" fullname=\"$_ENV[&#39;RUNLEVEL&#39;]\" address=\"139672244056120\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Mw==]]></property><property name=\"LANGSH_SOURCED\" fullname=\"$_ENV[&#39;LANGSH_SOURCED&#39;]\" address=\"139672244056304\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[MQ==]]></property><property name=\"PWD\" fullname=\"$_ENV[&#39;PWD&#39;]\" address=\"139672244056488\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Lw==]]></property><property name=\"LANG\" fullname=\"$_ENV[&#39;LANG&#39;]\" address=\"139672244056664\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Qw==]]></property><property name=\"previous\" fullname=\"$_ENV[&#39;previous&#39;]\" address=\"139672244056840\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Tg==]]></property><property name=\"PREVLEVEL\" fullname=\"$_ENV[&#39;PREVLEVEL&#39;]\" address=\"139672244057024\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Tg==]]></property><property name=\"CONSOLETYPE\" fullname=\"$_ENV[&#39;CONSOLETYPE&#39;]\" address=\"139672244057352\" type=\"string\" size=\"2\" encoding=\"base64\"><![CDATA[dnQ=]]></property><property name=\"SHLVL\" fullname=\"$_ENV[&#39;SHLVL&#39;]\" address=\"139672244057536\" type=\"string\" size=\"1\" encoding=\"base64\"><![CDATA[Mw==]]></property></property></response>";


	char msgBuffer[40960]; // used for writing resposnes to IntelliJ
	char commandBuffer[4096]; // used for reading commands from IntelliJ

	portno = 9000;
	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;

        // Assume serer=localhost unless server ip address is specified on command line.
        if (argc == 2) {
            //in_addr_t addr = inet_addr("10.8.131.168");
            printf("address to connect: %s\n", argv[1]);
            in_addr_t addr = inet_addr(argv[1]);
	    bcopy((char *) &addr,
			(char *)&serverAddress.sin_addr.s_addr,
			sizeof(addr));
        } else  {
  	    serverHost = gethostbyname("localhost");
	    bcopy((char *)serverHost->h_addr,
			(char *)&serverAddress.sin_addr.s_addr,
			serverHost->h_length);
        }
 
	serverAddress.sin_port = htons(portno);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	if (connect(sockfd,(struct sockaddr *) &serverAddress,sizeof(serverAddress)) < 0)
		error("ERROR connecting");

        printf("connected to IntelliJ listening port\n");

	const char *xmlInitStr = "<init xmlns=\"urn:debugger_protocol_v1\" xmlns:xdebug=\"http://xdebug.org/dbgp/xdebug\" fileuri=\"file:///box/www/ochotinun/index.php\" language=\"PHP\" protocol_version=\"1.0\" appid=\"20311\"><engine version=\"2.2.1\"><![CDATA[Xdebug]]></engine><author><![CDATA[Derick Rethans]]></author><url><![CDATA[http://xdebug.org]]></url><copyright><![CDATA[Copyright (c) 2002-2012 by Derick Rethans]]></copyright></init>";

	size_t msgLen = prepare_message(msgBuffer, xmlInitStr, -1);

	printf("\nSending intitial XML message of size %d %s\n\n", (int) msgLen, msgBuffer + 4);
	nwritten = write(sockfd, msgBuffer, msgLen);
	if (nwritten < 0)
	{
		error("ERROR writing to socket");
	}

	while(1) {
		printf("\n------------\nWaiting for command from Intellij...\n");
		// Read as many commands as are coming from IntelliJ
  	    bzero(commandBuffer, sizeof(commandBuffer));
		int nread = read(sockfd, commandBuffer, sizeof(commandBuffer));
		if (nread < 0)
		   	error("ERROR reading from socket");
		else if (nread == 0)
			error("IntelliJ closed the connection :( :( :(\n");
		else
			printf("received message of size %d from Intellij: \n%s\n", nread, commandBuffer);

		// nread bytes received can contain several commands separated by '\0'
		size_t msgLen = 0, sizeCommandsProcessed = 0;;
		const char *commandStartPtr = commandBuffer;
  	    bzero(msgBuffer, sizeof(msgBuffer));
		char *responseStartPtr = msgBuffer;

		while (sizeCommandsProcessed < nread)
		{
			size_t commandLen = strlen(commandStartPtr);
			printf("parsing command size %d of %d\n", (int) (commandLen + 1), (int) nread);

			IntelliJCommand *cmd = IntelliJCommand::parse(commandStartPtr);
			if (!cmd) {
				error("ERROR parsing command");
			}

			printf("Parsed IntelliJ msg: command = %s, transaction id = %d\n", cmd->getCmd(), cmd->getTransactionId());
    		const char *respStr = NULL;
    		if (strcmp(cmd->getCmd(), "feature_set") == 0)
    			respStr = featureResponses[numFeatureResponses++];
    		else if (strcmp(cmd->getCmd(), "status") == 0)
    			respStr = statusResponses[numStatusResponses++];
    		else if (strcmp(cmd->getCmd(), "step_into") == 0)
    			respStr = stepIntoResponses[numStepIntoResponses++];
    		else if (strcmp(cmd->getCmd(), "eval") == 0)
    			respStr = evalResponses[numEvalResponses++];
    		else if (strcmp(cmd->getCmd(), "run") == 0)
			{
				if (numRunResponses > 3)
				{
					printf("exceeded run responses\n");
					exit(-1);
				}
    			respStr = runResponses[numRunResponses++];
			}
    		else if (strcmp(cmd->getCmd(), "stack_get") == 0)
    			respStr = stackGetResponses[numStackGetResponses++];
    		else if (strcmp(cmd->getCmd(), "breakpoint_set") == 0)
    			respStr = breakpointResponses[numBreakpointResponses++];
			else if (strcmp(cmd->getCmd(), "context_names") == 0)
		    	respStr = contextNamesResponses[numContextNamesResponses++];
			else if (strcmp(cmd->getCmd(), "context_get") == 0)
		    	respStr = contextGetResponses[numContextGetResponses++];
			// todo(olga) handle "property_get" responses, at least 1
			else if (strcmp(cmd->getCmd(), "stop") == 0)
			{
		    	// todo(olga) generate response to stop.
		    	printf("stop received, exiting\n");
		    	exit(0);
			}
			else
		    	error("Unknown command from IntelliJ");
			msgLen += prepare_message(responseStartPtr, respStr, cmd->getTransactionId());
			responseStartPtr += msgLen;
			sizeCommandsProcessed += (commandLen + 1);
			commandStartPtr += (commandLen + 1);
		} // accumulate responses on to all commands received from IntelliJ

		printf("\nPress enter to send the above response to Intellij:\n");
		char c = fgetc(stdin);
		if (c == 'q' || c == 'Q')
		{
			printf("exiting\n");
			exit(0);
		}

		nwritten = write(sockfd, msgBuffer, msgLen);
		if (nwritten < 0)
			error("ERROR writing to socket");
	}

	close(sockfd);

	return 0;
    }
