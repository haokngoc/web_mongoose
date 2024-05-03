#include <iostream>
#include "sockpp/tcp_acceptor.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/cfg/env.h"
#include <fstream>
#include <json-c/json.h>
#include <thread>
#include "spdlog/sinks/rotating_file_sink.h"
#include <string>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include "mongoose.h"
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <string.h>
#include <json-c/json.h>

// Static variables
static int s_debug_level = MG_LL_INFO;
static const char *s_root_dir = ".";
static const char *s_listening_address = "http://0.0.0.0:9000";
static const char *s_enable_hexdump = "no";
static const char *s_ssi_pattern = ".html";
static const char *s_upload_dir = NULL;
static char *login_html;

static int s_signo;

// Signal handler
static void signal_handler(int signo) {
    s_signo = signo;
}
auto logger = spdlog::basic_logger_mt("logger", "logfile.txt");
int send_data_json_to_server() {
    // Open the data.json file to read data
    std::ifstream file("data.json", std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        logger->error("Error opening data.json file");
        return 1;
    }

    // Get the size of the file to allocate the correct size for the buffer
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate buffer and read data from file into buffer
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        logger->error("Error reading data.json file");
        return 1;
    }
    file.close();

    const char *server_ip = "127.0.0.1";
    const int server_port = 12345;
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        logger->error("Failed to create socket");
        return 1;
    }

    // Set up server address information
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
        logger->error("Failed to convert IP address");
        close(client_socket);
        return 1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        logger->error("Failed to connect to server");
        close(client_socket);
        return 1;
    }
    const char *dataCMD = "cmd1";
    ssize_t n_sent = send(client_socket, dataCMD, strlen(dataCMD), 0);
    sleep(2);
    // Send data to the server
    ssize_t bytes_sent = send(client_socket, buffer.data(), buffer.size(), 0);
    if (bytes_sent == -1) {
        logger->error("Error sending data to server");
        close(client_socket);
        return 1;
    }

    // Close the connection
    close(client_socket);
    return 0;
}

// Function to read login HTML content from file
static void read_login_html(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        logger->error("Failed to open login file");
        exit(EXIT_FAILURE);
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Read content into login_html variable
    login_html = (char *) malloc(file_size + 1);
    fread((void *) login_html, file_size, 1, file);
    login_html[file_size] = '\0';

    fclose(file);
}

// Authentication function
static int authenticate_user(const char *username, const char *password) {
    // Open the JSON file for reading
    FILE *file = fopen("data.json", "r");
    if (file == NULL) {
    	logger->error("Failed to open data.json file");
        return 0; // Return authentication error
    }

    // Create a JSON parser
    struct json_object *root = json_object_from_file("data.json");
    if (root == NULL) {
    	logger->error("Failed to parse data.json");
        fclose(file);
        return 0; // Return authentication error
    }

    // Find the "account_information" object
    struct json_object *account_info;
    if (!json_object_object_get_ex(root, "account_information", &account_info)) {
    	logger->error("Failed to find account_information object");
        fclose(file);
        json_object_put(root);
        return 0; // Return authentication error
    }

    // Retrieve user information from the "account_information" object
    struct json_object *username_obj, *password_obj;
    json_object_object_get_ex(account_info, "username", &username_obj);
    json_object_object_get_ex(account_info, "password", &password_obj);

    // Compare the username and password
    int authenticated = (strcmp(json_object_get_string(username_obj), username) == 0) &&
                        (strcmp(json_object_get_string(password_obj), password) == 0);

    // Free memory and close the file
    json_object_put(root);
    fclose(file);

    return authenticated; // Return authentication result
}


void update_json_data(const char *ip_address, const char *logging_level, const char *wireless_mode, const char *wireless_SSID, const char *wireless_Pass_Phrase) {
    FILE *file = fopen("data.json", "r+");
    printf("open file web!!!!!!!!\n");
    if (file == NULL) {
        perror("Failed to open data.json file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(fsize + 1);
    fread(file_content, 1, fsize, file);
    fclose(file);
    file_content[fsize] = '\0';

    struct json_object *root = json_tokener_parse(file_content);
    if (root == NULL) {
        perror("Error parsing JSON");
        free(file_content);
        return;
    }

    struct json_object *settings_obj;
    if (!json_object_object_get_ex(root, "settings", &settings_obj)) {
        perror("Error getting 'settings' object");
        json_object_put(root);
        free(file_content);
        return;
    }

    json_object_object_add(settings_obj, "ip-address", json_object_new_string(ip_address));
    json_object_object_add(settings_obj, "logging-level", json_object_new_string(logging_level));
    json_object_object_add(settings_obj, "wireless-mode", json_object_new_string(wireless_mode));
    json_object_object_add(settings_obj, "wireless-SSID", json_object_new_string(wireless_SSID));
    json_object_object_add(settings_obj, "wireless-Pass-Phrase", json_object_new_string(wireless_Pass_Phrase));

    file = fopen("data.json", "w");
    if (file == NULL) {
        perror("Failed to open data.json file");
        json_object_put(root);
        free(file_content);
        return;
    }

    fprintf(file, "%s", json_object_to_json_string(root));

    fclose(file);
    json_object_put(root);
    free(file_content);

    printf("close file web\n");
}


void update_password_in_json(const char *new_password) {
    FILE *file = fopen("/home/root/data.json", "r+");
    if (file == NULL) {
        logger->error("Failed to open data.json file");
        perror("Failed to open data.json file");
        return;
    }

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Read the content of the file into a buffer
    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(file);
        logger->error("Memory allocation failed");
        perror("Memory allocation failed");
        return;
    }
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    // Find the position of "password" in the JSON file
    char *password_start = strstr(buffer, "\"password\": \"");
    if (password_start != NULL) {
        password_start += strlen("\"password\": \"");
        char *password_end = strchr(password_start, '\"');
        if (password_end != NULL) {
            // Copy the new password into the old password's position
            strncpy(password_start, new_password, password_end - password_start);
        }
    } else {
        logger->error("Failed to find password field in JSON");
        printf("Failed to find password field in JSON\n");
    }

    // Set the file pointer to the beginning and rewrite the updated content
    rewind(file);
    fwrite(buffer, 1, file_size, file);

    // Free memory and close the file
    free(buffer);
    fclose(file);
}

// Event handler for HTTP connection
static void cb(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        // Handle HTTP messages
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_http_match_uri(hm, "/")) {
            // Redirect root URL to login page
            mg_http_reply(c, 200, "", "<html><head><script>window.location.href = 'login.html';</script></head><body></body></html>");
        }
        else if (mg_http_match_uri(hm, "/login")) {
            // Handle login page
            if (hm->method.len == 4 && memcmp(hm->method.ptr, "POST", 4) == 0) {
                // Process POST request for login
                char username[100], password[100];
                mg_http_get_var(&hm->body, "username", username, sizeof(username));
                mg_http_get_var(&hm->body, "password", password, sizeof(password));
                if (authenticate_user(username, password)) {
                    // Redirect to home page if authentication succeeds
                    mg_http_reply(c, 200, "", "<html><head><script>window.location.href = 'home.html';</script></head><body></body></html>");
                } else {
                    // Respond with authentication failed message
                    mg_http_reply(c, 401, "", "Authentication failed\n");
                }
            } else {
                // Serve login page for GET request
                mg_http_reply(c, 200, "", login_html);
            }
        }
        else if(mg_http_match_uri(hm,"/update")) {
            // Handle update endpoint for updating JSON data
            char ip_address[100], logging_level[100], wireless_mode[100], wireless_SSID[100], wireless_Pass_Phrase[100];
            mg_http_get_var(&hm->body, "ip-address", ip_address, sizeof(ip_address));
            mg_http_get_var(&hm->body, "logging-level", logging_level, sizeof(logging_level));
            mg_http_get_var(&hm->body, "wireless-mode", wireless_mode, sizeof(wireless_mode));
            mg_http_get_var(&hm->body, "wireless-SSID", wireless_SSID, sizeof(wireless_SSID));
            mg_http_get_var(&hm->body, "wireless-Pass-Phrase", wireless_Pass_Phrase, sizeof(wireless_Pass_Phrase));

            update_json_data(ip_address, logging_level, wireless_mode, wireless_SSID, wireless_Pass_Phrase);

            // Redirect to a success page or perform other actions if necessary
            // mg_http_reply(c, 200, "", "<html><head><script>window.location.href = 'settings.html';</script></head><body></body></html>");
            int tmp = send_data_json_to_server();
            const char *page;
            if(tmp == 1) {
                page = "<html><head><script>window.location.href = 'error.html';</script></head><body></body></html>";
            }
            else {
                page = "<html><head><script>window.location.href = 'settings.html';</script></head><body></body></html>";
            }
            mg_http_reply(c,200,"",page);

        }
        else if(mg_http_match_uri(hm,"/scanwf")) {

            const char *server_ip = "127.0.0.1";
            const int server_port = 12345;
            int client_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket == -1) {
                std::cerr << "Unable to create socket" << std::endl;
                return 1;
            }

            sockaddr_in server_address;
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(server_port);
            if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
                std::cerr << "Unable to convert IP address" << std::endl;
                close(client_socket);
                return 1;
            }

            if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
                std::cerr << "Unable to connect to server" << std::endl;
                close(client_socket);
                return 1;
            }
            const char *dataCMD = "cmd_scan_wifi";
            ssize_t n_gui = send(client_socket, dataCMD, strlen(dataCMD), 0);
            if (n_gui == -1) {
                std::cerr << "Error sending data to the server" << std::endl;
                close(client_socket);
                return 1;
            }
            char scanResults[1024];
            memset(scanResults, 0, sizeof(scanResults));
            ssize_t bytesRead = recv(client_socket, scanResults, sizeof(scanResults), 0);
            if (bytesRead == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            std::cout << "scanResults: " << scanResults << std::endl;
            close(client_socket);

//            const char *page = "<html><head><script>window.location.href = 'settings.html';</script></head><body></body></html>";
//            mg_http_reply(c,200,"",page);
            std::ifstream file("settings.html");
			std::string htmlContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

			std::string updatedHTML = htmlContent + "<div class='center'>" + scanResults + "</div>";

			mg_http_reply(c, 200, "", updatedHTML.c_str());
        }
        else if(mg_http_match_uri(hm,"/change_password")) {
            char password[100];
            // mg_http_get_var(&hm->body, "username", username, sizeof(username));
            mg_http_get_var(&hm->body, "new_password", password, sizeof(password));
            update_password_in_json(password);

            // Redirect to a success page or perform other actions if necessary
            mg_http_reply(c, 200, "", "<html><head><script>window.location.href = 'settings.html';</script></head><body></body></html>");
        }

        else if (mg_http_match_uri(hm, "/download")) {
            // This block handles the file download endpoint.
            // It determines the file name to be downloaded from the HTTP request.
            char file_name[100]; // Maximum length of the file name
            if (mg_http_get_var(&hm->body, "file_name", file_name, sizeof(file_name)) > 0) {
                // Construct the full path to the file to be downloaded
                char file_path[4096];
                snprintf(file_path, sizeof(file_path), "uploads/%s", file_name);

                // Open the file for reading
                FILE *downloaded_file = fopen(file_path, "rb");
                if (downloaded_file != NULL) {
                    // Seek to the end of the file to get its size
                    fseek(downloaded_file, 0, SEEK_END);
                    long file_size = ftell(downloaded_file);
                    rewind(downloaded_file);

                    // Send HTTP headers
                    mg_http_reply(c, 200, "Content-Disposition: attachment", "");

                    // Send the content of the file to the client
                    char *file_buffer = (char *)malloc(file_size);
                    if (file_buffer != NULL) {
                        fread(file_buffer, 1, file_size, downloaded_file);
                        mg_send(c, file_buffer, file_size);
                        free(file_buffer);
                    } else {
                        // Handle the error when memory allocation fails
                        mg_http_reply(c, 500, "", "Internal Server Error: Memory allocation failed\n");
                    }

                    fclose(downloaded_file);
                } else {
                    // Handle the error when the file fails to open for reading
                    mg_http_reply(c, 500, "", "Internal Server Error: Failed to open file for reading\n");
                }
            } else {
                // If no file name is specified in the request, return a 400 Bad Request error
                mg_http_reply(c, 400, "", "Bad Request: No file name specified\n");
            }
        }
		else if (mg_http_match_uri(hm, "/upload")) {
			struct mg_http_part part;
			size_t ofs = 0;

			FILE *fp = fopen("data.json", "r+");
			if (fp == NULL) {
				perror("Error opening data.json file");
				mg_http_reply(c, 500, "", "Internal Server Error");
				return;
			}

			char buffer[1024];
			size_t bytes_read = fread(buffer, 1, sizeof(buffer), fp);
			fclose(fp);

			if (bytes_read == 0) {
				perror("Error reading data.json file");
				mg_http_reply(c, 500, "", "Internal Server Error");
				return;
			}
			json_object *root_obj = json_tokener_parse(buffer);
			if (root_obj == NULL) {
				perror("Error parsing JSON data");
				mg_http_reply(c, 500, "", "Internal Server Error");
				return;
			}

			json_object *firmware_obj, *filename_obj;

			while ((ofs = mg_http_next_multipart(hm->body, ofs, &part)) > 0) {
				MG_INFO(("Chunk name: [%.*s] filename: [%.*s] length: %lu bytes",
						 (int) part.name.len, part.name.ptr, (int) part.filename.len,
						 part.filename.ptr, (unsigned long) part.body.len));

				printf("Chunk name: [%.*s] filename: [%.*s] length: %lu bytes\n",
					   (int) part.name.len, part.name.ptr, (int) part.filename.len,
					   part.filename.ptr, (unsigned long) part.body.len);
				firmware_obj = json_object_object_get(root_obj, "firmware");
				if (firmware_obj == NULL) {
					perror("Error accessing firmware object");
					mg_http_reply(c, 500, "", "Internal Server Error");
					json_object_put(root_obj);
					return;
				}

				filename_obj = json_object_object_get(firmware_obj, "filename");
				if (filename_obj == NULL) {
					perror("Error accessing filename object");
					mg_http_reply(c, 500, "", "Internal Server Error");
					json_object_put(root_obj);
					return;
				}

				json_object_set_string(filename_obj, part.filename.ptr);
			}
			fp = fopen("data.json", "w");
			if (fp == NULL) {
				perror("Error opening data.json file for writing");
				mg_http_reply(c, 500, "", "Internal Server Error");
				json_object_put(root_obj);
				return;
			}

			fputs(json_object_to_json_string(root_obj), fp);
			fclose(fp);
			json_object_put(root_obj);

			mg_http_reply(c, 302, "Location: /update_firmware.html", "");
		}


        else {
            struct mg_http_serve_opts opts = {0};
            opts.root_dir = s_root_dir;
            opts.ssi_pattern = s_ssi_pattern;
            mg_http_serve_dir(c, hm, &opts);
        }

        MG_INFO(("%.*s %.*s %lu -> %.*s %lu", hm->method.len, hm->method.ptr,
             hm->uri.len, hm->uri.ptr, hm->body.len, 3, c->send.buf + 9,
             c->send.len));
    }
}

// Usage function
static void usage(const char *prog) {
    fprintf(stderr,
            "Mongoose v.%s\n"
            "Usage: %s OPTIONS\n"
            "  -H yes|no - enable traffic hexdump, default: '%s'\n"
            "  -S PAT    - SSI filename pattern, default: '%s'\n"
            "  -d DIR    - directory to serve, default: '%s'\n"
            "  -l ADDR   - listening address, default: '%s'\n"
            "  -u DIR    - file upload directory, default: unset\n"
            "  -v LEVEL  - debug level, from 0 to 4, default: %d\n",
            MG_VERSION, prog, s_enable_hexdump, s_ssi_pattern, s_root_dir,
            s_listening_address, s_debug_level);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	// cout << get_current_dir() << endl;
    char path[MG_PATH_MAX] = ".";
    struct mg_mgr mgr;
    struct mg_connection *c;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            s_root_dir = argv[++i];
        } else if (strcmp(argv[i], "-H") == 0) {
            s_enable_hexdump = argv[++i];
        } else if (strcmp(argv[i], "-S") == 0) {
            s_ssi_pattern = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0) {
            s_listening_address = argv[++i];
        } else if (strcmp(argv[i], "-u") == 0) {
            s_upload_dir = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0) {
            s_debug_level = atoi(argv[++i]);
        } else {
            usage(argv[0]);
        }
    }

    if (strchr(s_root_dir, ',') == NULL) {
        realpath(s_root_dir, path);
        s_root_dir = path;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    mg_log_set(s_debug_level);
    mg_mgr_init(&mgr);
    if ((c = mg_http_listen(&mgr, s_listening_address, cb, &mgr)) == NULL) {
        MG_ERROR(("Cannot listen on %s. Use http://ADDR:PORT or :PORT", s_listening_address));
        exit(EXIT_FAILURE);
    }
    if (mg_casecmp(s_enable_hexdump, "yes") == 0) c->is_hexdumping = 1;

    MG_INFO(("Mongoose version : v%s", MG_VERSION));
    MG_INFO(("Listening on     : %s", s_listening_address));
    MG_INFO(("Web root         : [%s]", s_root_dir));
    MG_INFO(("Upload dir       : [%s]", s_upload_dir ? s_upload_dir : "unset"));
    read_login_html("login.html");
    while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    MG_INFO(("Exiting on signal %d", s_signo));
    return 0;
}

