/**
 ********************************************************************************************************
 * @file    main.cpp
 * @brief   MBD_get_weather_data
 * @details Downloads the weather data using the OpenWeather API
 ********************************************************************************************************
 */

#include <iostream>
#include <fstream>
#include <csignal>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#ifdef Boost_LIBRARIES_PRESENT
#include <boost/thread.hpp>
#endif

bool g_shutdown;


#ifdef Boost_LIBRARIES_PRESENT
void timedExit()
{
    std::chrono::time_point<std::chrono::system_clock>  start, end;
    std::chrono::duration<double>                       elapsed_seconds;

    start = std::chrono::system_clock::now();
    while (elapsed_seconds.count() < 15)
    {
        end = std::chrono::system_clock::now();
        elapsed_seconds = end - start;
    }

    exit(0);
}
#endif

void shutdown(int sig)
{
    g_shutdown = true;
#ifdef Boost_LIBRARIES_PRESENT
    boost::thread timed_exit(timedExit);
#endif
}

static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*) userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

int main(int argc, char *argv[])
{
    CURL                                                *curl;
    CURLcode                                            res;
    std::string                                         read_buffer;
    std::string                                         file_path;
    std::ofstream                                       output_file;
    std::string                                         weather_url;
    std::chrono::time_point<std::chrono::system_clock>  start, end;
    std::chrono::duration<double>                       elapsed_seconds;
    std::time_t                                         current_time;

    if (argc == 2)
    {
        file_path = argv[1];
        weather_url = "http://api.openweathermap.org/data/2.5/weather?q=Worcester&mode=xml";
    }
    else if (argc == 3)
    {
        file_path = argv[1];
        weather_url = argv[2];
    }
    else
    {
        file_path = "/home/nbanerjee/MBD_data/weather_data.xml";
        weather_url = "http://api.openweathermap.org/data/2.5/weather?q=Worcester&mode=xml";
    }

    g_shutdown = false;

    signal(SIGINT, shutdown);

    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, weather_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        while (g_shutdown == false)
        {
            res = curl_easy_perform(curl);

            if (res != CURLE_OK)
            {
                start = std::chrono::system_clock::now();
                current_time = std::chrono::system_clock::to_time_t(start);
                std::cout << "Failed update at " << std::ctime(&current_time) << "." << std::endl;
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res);
                continue;
            }

            output_file.open(file_path.c_str());
            output_file << read_buffer << std::endl;
            output_file.close();

            start = std::chrono::system_clock::now();
            current_time = std::chrono::system_clock::to_time_t(start);
            std::cout << "Updated at " << std::ctime(&current_time) << "." << std::endl;
            //std::cout << read_buffer << std::endl;

            // Wait for 10 mins before next update.
            while (g_shutdown == false)
            {
                end = std::chrono::system_clock::now();
                elapsed_seconds = end - start;
                if (elapsed_seconds.count() > 600)
                    break;
            }

            if (g_shutdown == false)
                continue;

            curl_easy_cleanup(curl);
            break;
        }
    }
    else
        std::cerr << "CURL error. curl_easy_init() failed.";

    std::cout << "\nExiting program." << std::endl;

    return 0;
}

// OpenWeatherMap
// APPID (API key)  fa454a139ab3917c2a5667a9f8df732e

//        curl_easy_setopt(curl, CURLOPT_URL, "http://api.openweathermap.org/data/2.5/weather?q=Worcester&mode=xml");
//        curl_easy_setopt(curl, CURLOPT_URL, "http://api.openweathermap.org/data/2.5/forecast/city?id=4956184&APPID=fa454a139ab3917c2a5667a9f8df732e&mode=xml");
