#include "proxy.h"

class Parse {
private:
    
    //Format: Sun, 23 Feb 2020 20:07:07 GMT
    static time_t getTime(std::string input) {
        unsigned long start = input.find(", ")+2;
        unsigned long end = input.find(" GMT");
        input = input.substr(start, end - start);
        input = input.substr(input.find(", ") + 2);
        struct tm tm;
        std::time_t t;
        strptime(input.c_str(), "%d %b %Y %H:%M:%S", &tm);
        t = mktime(&tm);
        return t;
    }
    
public:
    static int transferStringToInt(std::string number) {
        int num = 0;
        for (char c: number) {
            if (c >= '0' && c <= '9') {
                int n = c - '0';
                num = num * 10 + n;
            } else {
                return num;
            }
        }
        return num;
    }

    /*
     * Read a char array from the input
     * Divide the request into three parts with key:ACTION, WEB, HTTP, HOST
     */
    static std::unordered_map<std::string, std::string> parseRequest(std::string input, std::string IP) {
        std::unordered_map<std::string, std::string> result;
        
        //get the client's IP
        result["CLIENTIP"] = IP;

        //get the action
        unsigned long actionPos = input.find(" ");
        std::string action = input.substr(0, actionPos);
        result["ACTION"] = action;

        //get the web
        std::string curr = input.substr(actionPos+1);
        unsigned long webPos = curr.find(" ");
        std::string web = curr.substr(0, webPos);
        result["WEB"] = web;

        //get the http
        curr = curr.substr(webPos+1);
        unsigned long httpPos = curr.find("\r\n");
        std::string http = curr.substr(0, httpPos);
        result["HTTP"] = http;
        
        //get the host
        std::string hostDelimiter = "Host: ";
        unsigned long hostPos = input.find(hostDelimiter);
        std::string host = input.substr(hostPos + hostDelimiter.length());
        host = host.substr(0, host.find("\r\n"));
        host.append("\0");
        result["HOST"] = host;
        
        //get the time
	std::time_t time = std::time(nullptr);
	char *dt = std::ctime(&time);
        result["TIME"] = dt;
        return result;
    }
    
    /*
     Read the header and return the number in it.
     Return a pair of string and size_t.
     The first one is "CHUNKED" or "NONCHUNKED".
     If the first one is "NONCHUNKED", the second is the total size.
     */
    static std::pair<std::string, size_t> parseContentLength(std::vector<char> inputChar) {
        std::pair<std::string, size_t> result;
        std::string input(inputChar.begin(), inputChar.end());
        if (input.find("chunked") != std::string::npos) {
            result.first = "CHUNKED";
            result.second = 0;
            return result;
        }
        result.first = "NONCHUNKED";
        std::string delimiter = "Content-Length: ";
        unsigned long pos = input.find(delimiter) + delimiter.length();
        if (pos == std::string::npos) {
            result.second = 0;
            return result;
        }
        std::string number = input.substr(pos);
        int bodyLength = transferStringToInt(number);
        unsigned long headerPos = input.find("\r\n\r\n");
        result.second = bodyLength + headerPos + 4;
        return result;
    }
    
    /*
     Get Cache Control of the resource.
     If there is no key word "Cache-Control: ", return -1.
     Else, return the number.
     */
    static long parseCacheControl(std::vector<char> inputChar) {
        std::string input(inputChar.begin(), inputChar.end());
        if (input.find("Cache-Control: ") != std::string::npos) {
            if (input.find("no-cache") != std::string::npos)
            {
                return 0;
            }
            std::string delimiter = "max-age=";
            unsigned long pos = input.find(delimiter) + delimiter.length();
            std::string number = input.substr(pos);
            return transferStringToInt(number);
        }
        
        std::string dateDelimiter = "Date: ";
        unsigned long datePos = input.find(dateDelimiter);
        time_t date;
        if (datePos == std::string::npos) {
            date = time_t(nullptr);
        } else {
            date = getTime(input.substr(datePos+dateDelimiter.length(), input.find("\n") - datePos));
        }
        
        std::string expiresDelimiter = "Expires: ";
        unsigned long expiresPos = input.find(expiresDelimiter);
        if (expiresPos != std::string::npos) {
            time_t expires = getTime(input.substr(expiresPos+expiresDelimiter.length(), input.find("\n") - expiresPos));
            return expires - date;
        }
        
        std::string modifyDelimiter = "Last-Modified: ";
        unsigned long modifyPos = input.find(modifyDelimiter);
        if (modifyPos != std::string::npos) {
            time_t modify = getTime(input.substr(modifyPos+modifyDelimiter.length(), input.find("\n") - modifyPos));
            return (date - modify)/10;
        }
        
        return -1;
    }

    /*                                                                                        Return the eTag.
     */
    static std::string parseETag(std::vector<char> inputChar) {
        std::string input(inputChar.begin(), inputChar.end());
        std::string result;
        std::string delimiter = "ETag: ";
        unsigned long pos = input.find(delimiter);
        if (pos != std::string::npos) {
            //if there is an eTag
            input = input.substr(pos + delimiter.length());
            std::string eTag = input.substr(0, input.find("\r\n"));
            result.append("\r\nIf-None-Match: ");
            result.append(eTag);
            return result;
        }
        //if there is no etag, find the last modified time
        std::string modifyDelimiter = "Last-Modified: ";
        unsigned long modifyPos = input.find(modifyDelimiter);
        input = input.substr(modifyPos + modifyDelimiter.length());
        std::string time = input.substr(0, input.find("\r\n"));
        result.append("\r\nIf-Modified-Since: ");
        result.append(time);
        return result;
    }

    /*
     Return the number.
     */
    static std::string parseResponseNumber(std::vector<char> inputChar) {
        std::string input(inputChar.begin(), inputChar.end());
        unsigned long pos = input.find(" ") + 1;
        input = input.substr(pos);
        return input.substr(0, input.find("\r\n"));
    }

};
