#include "proxy.h"

class Response {
private:
    std::string message;
    std::string eTag;
    std::time_t birth;
    bool isNULL=true;
    long lifeSpan;
    int requestID;
    int length;
    
public:
    Response() {
    }
    
    Response(std::vector<char> m, int id): birth(time(nullptr)) {
        std::string dummy(m.begin(), m.end());
        message = dummy;
        isNULL = false;
        requestID = id;
    }
    
    /*                                                                                                                                                 
    Check if needs revalidate                                                                                                                          
    */
    std::string isValid() {
        //if EXPIREDTIME                                                                                                                               
        if (lifeSpan == 0) {
            return "REVALIDATION";
        }
        std::time_t curr = time(nullptr);
        if (curr - birth >= lifeSpan || lifeSpan < 0) {
            //if needs revalidation                                                                                              
            return "EXPIREDTIME";
        }
        //if valid                                                                                                                                     
        return "VALID";
    }
        
    //set
    void setETag(std::string etag) {
        eTag = etag;
    }
    
    void setBirth(std::time_t time) {
        birth = time;
    }
    
    void setLifeSpan(long length) {
        lifeSpan = length;
    }
    
    
    
    //get
    std::string getMessage() {
        return message;
    }
    std::string getETag() {
        return eTag;
    }
    
    int getRequestID() {
        return requestID;
    }
    
    std::time_t getBirth() {
        return birth;
    }
    
    bool getIsNULL() {
        return isNULL;
    }
};