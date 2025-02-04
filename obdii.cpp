#include <boost/asio.hpp> 
#include <boost/asio/buffer.hpp>
#include <iostream> 
#include <thread>
    
using boost::asio::ip::tcp;

class OBDIIReader 
{
    private:
        tcp::socket socket_;

    public:
        OBDIIReader(boost::asio::io_context& io_context, const std::string& ip, const std::string& port)
        : socket_(io_context){
            tcp::resolver resolver(io_context);
            boost::asio::connect(socket_, resolver.resolve(ip, port));
        }

        std::string sendcommand(const std::string& command){
            boost::asio::write(socket_, boost::asio::buffer(command + "\r"));
            boost::asio::streambuf response;
            boost::asio::read_until(socket_, response, "\r");
            std::string result((std::istreambuf_iterator<char>(&response)), std::istreambuf_iterator<char>());
            return result;
        }

        int getRPM(){
            std::string response = sendcommand("010C"); //code for rpm, given in >1 0C 0A A6
            std::cout << response << std::endl;

            response.erase(std::remove(response.begin(), response.end(), '>'), response.end());
            response.erase(std::remove(response.begin(), response.end(), ' '), response.end());

            if(response.length() >= 6){ 
                try{
                    std::cout << response << std::endl;
                    std::cout << response.substr(static_cast<int>(response.length())-6,2) << std::endl;
                    std::cout << response.substr(static_cast<int>(response.length())-4,2) << std::endl;

                    std::string ahex = response.substr(static_cast<int>(response.length())-6,2);
                    std::string bhex = response.substr(static_cast<int>(response.length())-4,2);
                    
                    int a = std::stoi(ahex, nullptr, 16);
                    int b = std::stoi(bhex, nullptr, 16);
                    
                    int rpm = ((a*256)+b)/4;
                    return rpm;
                }
                catch(const std::invalid_argument& e){
                    std::cerr << "ERROR: Invalid RPM Response Format";
                    return -1;
                }
            }
            std::cerr << "ERROR: RPM Response Too Short";
            return -1;
        }

        int getSPEED(){
            std::string response = sendcommand("010D");
            std::cout << response << std::endl;

            response.erase(std::remove(response.begin(), response.end(), '>'), response.end());
            response.erase(std::remove(response.begin(), response.end(), ' '), response.end());

            if(response.length() >= 4){
                try{
                    std::string hexval = response.substr(static_cast<int>(response.length())-4,2);
                    int kmh = std::stoi(hexval, nullptr, 16);
                    int mph = static_cast<int>(kmh * 0.621371);
                    return mph;
                }
                catch(const std::invalid_argument& e){
                    std::cerr << "ERROR: Invalid Speed Response Format";
                    return -1;
                }
            }
            return -1;
        }

                int getCOOLANTTEMP(){
            std::string response = sendcommand("0105");
            std::cout << response << std::endl;

            response.erase(std::remove(response.begin(), response.end(), '>'), response.end());
            response.erase(std::remove(response.begin(), response.end(), ' '), response.end());

            if(response.length() >= 4){
                try{
                    std::string hexval = response.substr(static_cast<int>(response.length())-3,2);
                    int temp = std::stoi(hexval, nullptr, 16) - 40;
                    return temp;
                }
                catch(const std::invalid_argument& e){
                    std::cerr << "ERROR: Invalid Coolant Response Format";
                    return -1;
                }
            }
            return -1;
        }

};

int main(){
    try{
        boost::asio::io_context io_context;
        OBDIIReader reader(io_context, "192.168.0.10", "35000"); //change values for reader 

        while(true){
            int rpm = reader.getRPM();
            if(rpm != -1){
                std::cout << "RPM: " << rpm << std::endl;
            }
            else{
                std::cout << "RPM Failed" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            int mph = reader.getSPEED();
            if(mph != -1){
                std::cout << "MPH: " << mph << std::endl;
            }
            else{
                std::cout << "MPH Failed" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch(std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
