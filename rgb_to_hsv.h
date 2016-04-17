// From: http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_(C)

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

struct hsv_color rgb_to_hsv (struct rgb_color);
