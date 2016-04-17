
// Smarthome Controller
// (c) Hunter Forsyth 2016


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cJSON.h"
#include "config.h"

#define BUFFER_SIZE 1024

// HSV color converter below from:
// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_(C)
struct rgb_color {
    double r, g, b;    /* Channel intensities between 0.0 and 1.0 */
};
struct hsv_color {
    double hue;        /* Hue degree between 0.0 and 360.0 */
    double sat;        /* Saturation between 0.0 (gray) and 1.0 */
    double val;        /* Value between 0.0 (black) and 1.0 */
};
#define MIN3(x,y,z)  ((y) <= (z) ? \
                         ((x) <= (y) ? (x) : (y)) \
                     : \
                         ((x) <= (z) ? (x) : (z)))
#define MAX3(x,y,z)  ((y) >= (z) ? \
                         ((x) >= (y) ? (x) : (y)) \
                     : \
                         ((x) >= (z) ? (x) : (z)))
struct hsv_color rgb_to_hsv(struct rgb_color rgb) {
    struct hsv_color hsv;
    double rgb_min, rgb_max;
    rgb_min = MIN3(rgb.r, rgb.g, rgb.b);
    rgb_max = MAX3(rgb.r, rgb.g, rgb.b);
    hsv.val = rgb_max;
    if (hsv.val == 0) {
        hsv.hue = hsv.sat = 0;
        return hsv;
    }
    /* Normalize value to 1 */
    rgb.r /= hsv.val;
    rgb.g /= hsv.val;
    rgb.b /= hsv.val;
    rgb_min = MIN3(rgb.r, rgb.g, rgb.b);
    rgb_max = MAX3(rgb.r, rgb.g, rgb.b);
    hsv.sat = rgb_max - rgb_min;
    if (hsv.sat == 0) {
        hsv.hue = 0;
        return hsv;
    }
    /* Normalize saturation to 1 */
    rgb.r = (rgb.r - rgb_min)/(rgb_max - rgb_min);
    rgb.g = (rgb.g - rgb_min)/(rgb_max - rgb_min);
    rgb.b = (rgb.b - rgb_min)/(rgb_max - rgb_min);
    rgb_min = MIN3(rgb.r, rgb.g, rgb.b);
    rgb_max = MAX3(rgb.r, rgb.g, rgb.b);
    /* Compute hue */
    if (rgb_max == rgb.r) {
        hsv.hue = 0.0 + 60.0*(rgb.g - rgb.b);
        if (hsv.hue < 0.0) {
            hsv.hue += 360.0;
        }
    } else if (rgb_max == rgb.g) {
        hsv.hue = 120.0 + 60.0*(rgb.b - rgb.r);
    } else /* rgb_max == rgb.b */ {
        hsv.hue = 240.0 + 60.0*(rgb.r - rgb.g);
    }

    // Modified to support hue's color format:
    hsv.hue = (hsv.hue/360) * 65535;
    hsv.sat = hsv.sat * 254;
    hsv.val = hsv.val * 254;

    return hsv;
}

// From: https://gist.github.com/nolim1t/126991
int socket_connect(char *host, in_port_t port){
    struct hostent *hp;
    struct sockaddr_in addr;
    int on = 1, sock;

    if((hp = gethostbyname(host)) == NULL){
        herror("gethostbyname");
        exit(1);
    }

    bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(sock == -1){
        perror("setsockopt");
        exit(1);
    }

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        exit(1);
    }

    return sock;
}

/* Send a PUT or GET to BRIDGE_IP/api/USER/%p, where %p is the 'path' arg below.
   If 'put_req' is 1 it will be a PUT request, otherwise it will be a GET request.
   Specify a json to send in the 'body' arg for a PUT request.
   'body' can be NULL for a GET request.
   Return the response.
*/
char* send_req(char* path, char* body, int put_req){

    // Connect to the socket:
    int fd;
    fd = socket_connect(BRIDGE_IP, 80);

    char json[256];
    char len_str[15];
    if (put_req == 1){
        // Construct the json:
        strcpy(json, body);
        int len = strlen(json);
        sprintf(len_str, "%d", len);
    }

    // Construct the request:
    char req[256];
    if (put_req == 1){strcpy(req, "PUT /api/");}
    else             {strcpy(req, "GET /api/");}
    strcat(req, USER);
    strcat(req, path);
    strcat(req, " HTTP/1.0\r\n");
    if (put_req == 1){
        strcat(req, "Content-Type: text/json\r\nContent-Length: ");
        strcat(req, len_str);
        strcat(req, "\r\n\r\n");
        strcat(req, json);
    } else {
        strcat(req, "\r\n");
    }

    write(fd, req, strlen(req));

    // Retrieve the response:
    static char response[4096];
    char buffer[BUFFER_SIZE];
    int i = 0;
    bzero(buffer, BUFFER_SIZE);
    while(read(fd, buffer, BUFFER_SIZE - 1) != 0){
        if (i > 0){
            strcat(response, buffer);
        } else {
            strcpy(response, buffer);
        }
        bzero(buffer, BUFFER_SIZE);
        i++;
    }

    shutdown(fd, SHUT_RDWR);
    close(fd);

    return response;

}

/* Set the light specified by 'lightnum' to r,g,b values.
   Also controls the light's on/off state automatically.
*/
void control_light(int lightnum, int r, int g, int b){

    // Turn light on iff color specified.
    int on_off = 1;
    if (r == 0 && g == 0 && b == 0){
        on_off = 0;
    }

    // Account for incorrect values.
    if (r > 254){ r = 254; }
    if (g > 254){ g = 254; }
    if (b > 254){ b = 254; }
    if (r == g && g == b){ r--; }
    if (r < 0){ r = 0; }
    if (g < 0){ g = 0; }
    if (b < 0){ b = 0; }

    // Light number string:
    char lightnum_str[15];
    sprintf(lightnum_str, "%d", lightnum);

    // On/Off string:
    char on_off_string[15];
    sprintf(on_off_string, "%s", on_off ? "true" : "false");

    // Build the request:
    char req[256];
    strcpy(req, "/lights/");
    strcat(req, lightnum_str);
    strcat(req, "/state");

    // Build the color:
    struct rgb_color rgb;
    rgb.r = (double) r / 255.0;
    rgb.g = (double) g / 255.0;
    rgb.b = (double) b / 255.0;
    struct hsv_color hsv = rgb_to_hsv(rgb);
    int h = (int) hsv.hue;
    char h_str[15];
    sprintf(h_str, "%d", h);
    int s = (int) hsv.sat;
    char s_str[15];
    sprintf(s_str, "%d", s);
    int v = (int) hsv.val;
    char v_str[15];
    sprintf(v_str, "%d", v);

    // Build the json:
    char json[256];
    strcpy(json, "{\"on\": ");
    strcat(json, on_off_string);
    strcat(json, ", \"sat\": ");
    strcat(json, s_str);
    strcat(json, ", \"bri\": ");
    strcat(json, v_str);
    strcat(json, ", \"hue\": ");
    strcat(json, h_str);
    strcat(json, "}");

    send_req(req, json, 1);

}

void list_lights(){

    // Query lights with a GET request:
    char* req = "/lights";
    char* res = send_req(req, NULL, 0);

    // Separate the response JSON from the header:
    char* token;
    char json[4096];
    token = strtok(res, "\r\n");
    while (token != NULL){
          strcpy(json, token);
          token = strtok(NULL, "\r\n");
    }

    // Parse the response JSON:
    cJSON* root = cJSON_Parse(json);
    cJSON* light;
    if (root != NULL){
        int i = 1;
        char light_index[3];
        sprintf(light_index, "%d", i);
        while ((light = cJSON_GetObjectItem(root, light_index)) != NULL){
            char* light_name = cJSON_GetObjectItem(light,"name")->valuestring;
            if (light_name != NULL){
                printf("%s: %s\n", light_index, light_name);
            }
            i++;
            sprintf(light_index, "%d", i);
            if (i > 20){
                break;
            }
        }
    }


}

int main(int argc, char *argv[]){

    // Check args:
    if(argc < 2){
        printf("Try home --help\n");
        exit(1);
    }
    if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "help") == 0){
        printf("\nSmarthome controller\n(c) 2016 Hunter Forsyth\n\n");
        printf("Currently supports Philips Hue. Bridge IP and user key are inside config.h\n\n");
        printf("Usage:\n\n");

        printf("  home listlights\n");
        printf("  - List each light by their lightnum.\n\n");

        printf("  home light [lightnum] [r],[g],[b]\n");
        printf("  - Set a single light's color.\n");
        printf("  - For example:\n    $ home light 1 255,0,255\n\n");

        printf("  home lights [lightnum_1],[lightnum_2],...,[lightnum_n] [r],[g],[b]\n");
        printf("  - Set the color of multiple lights.\n");
        printf("  - For example:\n    $ home lights 1,2,3,4 255,255,255");
        printf("\n    $ home lights all red\n\n");

        printf("  An r,g,b color can be replaced with one of the keywords:\n");
        printf("  {on, off, red, blue, green, white, orange, yellow, purple, pink, reading}\n\n");
    }

    // List lights:
    if(strcmp(argv[1], "listlights") == 0){
        list_lights();
    }

    // Send a light request:
    if(strcmp(argv[1], "light") == 0 || strcmp(argv[1], "lights") == 0){
        if (argc != 4){
            printf("Try home --help\n");
            exit(1);
        }
        int lightarray[10];
        int lights = 0;
        int lightnum = -1;
        if (strcmp(argv[1], "light") == 0){ // Single light
            lightnum = atoi(argv[2]);
        } else { // Multiple lights
            if (strcmp(argv[2], "all") == 0){
                while (lights < 10){
                    lightarray[lights] = lights;
                    lights++;
                }
            } else {
                char* ltoken;
                while ((ltoken = strsep(&argv[2], ",")) != NULL){
                    lightarray[lights] = atoi(ltoken);
                    lights++;
                    if (lights >= 10){
                        break;
                    }
                }
            }
        }

        char* token;
        int r = -1;
        int g = -1;
        int b = -1;

        // Color keywords
        int used_keyword = 0;
        if ((strcmp(argv[3], "off")) == 0){ // OFF
            used_keyword = 1;
            r = 0; g = 0; b = 0;
        } else if ((strcmp(argv[3], "on")) == 0){ // ON
            used_keyword = 1;
            r = 255; g = 255; b = 255;
        } else if ((strcmp(argv[3], "red")) == 0){ // RED
            used_keyword = 1;
            r = 255; g = 0; b = 0;
        } else if ((strcmp(argv[3], "blue")) == 0){ // BLUE
            used_keyword = 1;
            r = 0; g = 0; b = 255;
        } else if ((strcmp(argv[3], "green")) == 0){ // GREEN
            used_keyword = 1;
            r = 0; g = 255; b = 0;
        } else if ((strcmp(argv[3], "white")) == 0){ // WHITE
            used_keyword = 1;
            r = 255; g = 255; b = 255;
        } else if ((strcmp(argv[3], "orange")) == 0){ // ORANGE
            used_keyword = 1;
            r = 255; g = 200; b = 0;
        } else if ((strcmp(argv[3], "yellow")) == 0){ // YELLOW
            used_keyword = 1;
            r = 220; g = 255; b = 0;
        } else if ((strcmp(argv[3], "reading")) == 0){ // READING
            used_keyword = 1;
            r = 200; g = 255; b = 100;
        } else if ((strcmp(argv[3], "purple")) == 0){ // PURPLE
            used_keyword = 1;
            r = 100; g = 0; b = 255;
        } else if ((strcmp(argv[3], "pink")) == 0){ // PINK
            used_keyword = 1;
            r = 255; g = 100; b = 255;
        } else {
            // RGB color
            if ((token = strsep(&argv[3], ",")) != NULL){
                r = atoi(token);
            }
            if ((token = strsep(&argv[3], ",")) != NULL){
                g = atoi(token);
            }
            if ((token = strsep(&argv[3], ",")) != NULL){
                b = atoi(token);
            }
        }
        if (r >= 0 && g >= 0 && b >= 0 && (strsep(&argv[3], ",") == NULL || used_keyword == 1)){
            if (lightnum >= 0){ // Single light
                control_light(lightnum, r, g, b);
            } else { // Multiple lights
                for (int i = 0; i < lights; i++){
                    control_light(lightarray[i], r, g, b);
                }
            }
        } else {
            printf("Try: home help\n");
            exit(1);
        }
    }

    return 0;
}
