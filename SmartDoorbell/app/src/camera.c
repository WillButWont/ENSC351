/**
 * @file camera.c
 * @brief Handles image capture and motion detection logic.
 * * This module downloads JPEG images from the ESP32-CAM via HTTP (wget),
 * decodes them into RGB buffers, and compares sequential frames to detect
 * significant changes (motion). It uses a simple background subtraction
 * algorithm with a running average update.
 */

#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <string.h>
#include <math.h>

// --- Configuration ---
#define IMG_PATH "/tmp/visitor.jpg" // File path for the downloaded image. /tmp is usually a RAM disk, reducing flash wear/latency.
#define MOTION_THRESH 0.15          // Threshold: if >15% of pixels change, motion is detected.
#define PIXEL_THRESH 60             // Sensitivity: Minimum RGB difference (0-255) to consider a pixel "changed".

// --- State Variables ---
static unsigned char* bg_buffer = NULL; // Buffer holding the "background" (previous) frame for comparison.
static int img_w = 0, img_h = 0;        // Dimensions of the current video stream.

/**
 * @brief Helper function to decode a JPEG file into a raw RGB byte array.
 * * Uses libjpeg to decompress the image.
 * * @param filename Path to the JPEG file.
 * @param w Pointer to store the output width.
 * @param h Pointer to store the output height.
 * @return unsigned char* Pointer to the allocated RGB buffer (must be freed by caller), or NULL on failure.
 */
unsigned char* load_jpeg(const char* filename, int* w, int* h) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    // Open the image file
    FILE* infile = fopen(filename, "rb");
    if (!infile) return NULL;

    // Initialize the JPEG decompression object with default error handling
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    
    // Read file header to get image info (width/height)
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *w = cinfo.output_width;
    *h = cinfo.output_height;
    
    // Allocate memory for the raw RGB data (width * height * 3 bytes per pixel)
    unsigned char* buf = malloc((*w) * (*h) * cinfo.output_components);
    unsigned char* rowptr[1]; // Pointer array for the scanline

    // Read scanlines one by one into the buffer
    while (cinfo.output_scanline < cinfo.output_height) {
        // Calculate the pointer to the current row in our buffer
        rowptr[0] = &buf[cinfo.output_scanline * (*w) * cinfo.output_components];
        jpeg_read_scanlines(&cinfo, rowptr, 1);
    }
    
    // Clean up libjpeg resources and file handle
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return buf;
}

/**
 * @brief Initialize the camera module.
 * * Clears any stale images from the temp directory to ensure a clean state start.
 */
void camera_init(void) { system("rm -f " IMG_PATH); }

/**
 * @brief Capture a still image from the ESP32-CAM.
 * * Executes a shell command (wget) to download the image. 
 * Includes a timeout to prevent the main loop from hanging if the camera is offline.
 * * @param ip The IP address of the ESP32-CAM.
 * @return int Status code from system() call (0 usually means success).
 */
int camera_capture(const char* ip) {
    char cmd[256];
    // Construct the command: 
    // -q: Quiet mode (no output)
    // -O: Output filename
    // -T 1: 1-second timeout
    snprintf(cmd, sizeof(cmd), "wget -q -O %s -T 1 http://%s/still", IMG_PATH, ip);
    return system(cmd);
}

/**
 * @brief Analyze the captured image for motion.
 * * Compares the latest downloaded image against a stored background buffer.
 * If pixels differ by more than PIXEL_THRESH, they count as "changed".
 * If the total percentage of changed pixels exceeds MOTION_THRESH, motion is reported.
 * The background is also updated using a running average to adapt to lighting changes.
 * * @return true if motion is detected, false otherwise.
 */
bool camera_check_motion(void) {
    int w, h;
    // Decode the image downloaded by camera_capture()
    unsigned char* curr = load_jpeg(IMG_PATH, &w, &h);
    if (!curr) return false;

    // Initialize background if empty or if image dimensions changed
    if (!bg_buffer || w != img_w || h != img_h) {
        if (bg_buffer) free(bg_buffer);
        bg_buffer = curr;   // Set current frame as the new baseline
        img_w = w; img_h = h;
        return false;   // Cannot detect motion on the very first frame
    }

    long diff_count = 0;
    long total_bytes = w * h * 3; // Total bytes (RGB)

    for (long i = 0; i < total_bytes; i+=3) { // Check RGB
        int diff = abs(curr[i] - bg_buffer[i]);
        if (diff > PIXEL_THRESH) diff_count++;

        // Update background using Running Average Algorithm
        // This slowly blends the current frame into the background (80% old, 20% new)
        // allowing the system to adapt to slow lighting changes (e.g., sun setting)
        // without triggering false positives, while fast changes (people) trigger motion.
        bg_buffer[i] = (unsigned char)((bg_buffer[i]*0.8) + (curr[i]*0.2));
    }
    
    // Current frame is no longer needed (background buffer persists)
    free(curr);

    // Return true if the ratio of changed pixels exceeds the defined threshold
    return ((float)diff_count / (float)(w*h)) > MOTION_THRESH;
}

/**
 * @brief Cleanup camera resources.
 * Frees the persistent background buffer used for motion detection.
 */
void camera_cleanup(void) { if(bg_buffer) free(bg_buffer); }