#include <unistd.h>
#include <thread>
#include <GLUT/glut.h>

#include "ps4eye.h"

using namespace std;

int height, width;
int frame_rgb_size = 0;
uint8_t *frame_rgb_left;
uint8_t *frame_rgb_right;
ps4eye::PS4EYECam::PS4EYERef camera;
GLuint texture_left, texture_right;

void yuv2rgb(int y, int u, int v, char *r, char *g, char *b)
{
    int r1, g1, b1;
    int c = y-16, d = u - 128, e = v - 128;

    r1 = (298 * c           + 409 * e + 128) >> 8;
    g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
    b1 = (298 * c + 516 * d           + 128) >> 8;

    // Even with proper conversion, some values still need clipping.

    if (r1 > 255) r1 = 255;
    if (g1 > 255) g1 = 255;
    if (b1 > 255) b1 = 255;
    if (r1 < 0) r1 = 0;
    if (g1 < 0) g1 = 0;
    if (b1 < 0) b1 = 0;

    *r = r1 ;
    *g = g1 ;
    *b = b1 ;
}

void yuyvToRgb(const uint8_t *in,uint8_t *out, int size_x,int size_y)
{
    int i;
    unsigned int *pixel_16=(unsigned int*)in;;     // for YUYV
    unsigned char *pixel_24=out;    // for RGB
    int y, u, v, y2;
    char r, g, b;


    for (i=0; i< (size_x*size_y/2) ; i++)
    {
        // read YuYv from newBuffer (2 pixels) and build RGBRGB in pBuffer

     //   v  = ((*pixel_16 & 0x000000ff));
       // y  = ((*pixel_16 & 0x0000ff00)>>8);
       // u  = ((*pixel_16 & 0x00ff0000)>>16);
       // y2 = ((*pixel_16 & 0xff000000)>>24);

        y2  = ((*pixel_16 & 0x000000ff));
        u  = ((*pixel_16 & 0x0000ff00)>>8);
        y  = ((*pixel_16 & 0x00ff0000)>>16);
        v = ((*pixel_16 & 0xff000000)>>24);

    yuv2rgb(y, u, v, &r, &g, &b);            // 1st pixel


        *pixel_24++ = r;
        *pixel_24++ = g;
        *pixel_24++ = b;



    yuv2rgb(y2, u, v, &r, &g, &b);            // 2nd pixel

        *pixel_24++ = r;
        *pixel_24++ = g;
        *pixel_24++ = b;



        pixel_16++;
    }
}

void convert_YUYV_to_RGBA8(const uint8_t *yuyv,  uint8_t *rgb, int size_x, int size_y)
{
    int K1 = (1.402f * (1 << 16));
    int K2 = (0.714f * (1 << 16));
    int K3 = (0.334f * (1 << 16));
    int K4 = (1.772f * (1 << 16));

    unsigned char* out_ptr = rgb;

    // unsigned char* out_ptr = &rgb_image[1280*3-1];
    const int pitch = size_x * 2; // 2 bytes per one YU-YV pixel

    for (int y=0; y<size_y; y++) {
        out_ptr=&rgb[size_x * 4*y];
        const unsigned char * src = yuyv + pitch * y;
        for (int x=0; x < pitch; x += 4) { // Y1 U Y2 V
            //for (int x=size_x*2-1; x<=0; x-=4) { // Y1 U Y2 V

            unsigned char Y1 = src[x + 0];
            unsigned char U  = src[x + 1];
            unsigned char Y2 = src[x + 2];
            unsigned char  V  = src[x + 3];

            int uf = U - 128;
            int vf = V - 128;

            int R = Y1 + (K1*vf >> 16);
            int G = Y1 - (K2*vf >> 16) - (K3*uf >> 16);
            int B = Y1 + (K4*uf >> 16);

            if (R < 0) R = 0;
            else if (R > 255) R = 255;
            if (G < 0) G = 0;
            else if (G > 255) G = 255;
            if (B < 0) B = 0;
            else if (B > 255) B = 255;

            *out_ptr++ = (unsigned char)R;
            *out_ptr++ = (unsigned char)G;
            *out_ptr++ = (unsigned char)B;
            *out_ptr++ = 255;

            R = Y2 + (K1*vf >> 16);
            G = Y2 - (K2*vf >> 16) - (K3*uf >> 16);
            B = Y2 + (K4*uf >> 16);

            if (R < 0) R = 0;
            else if (R > 255) R = 255;
            if (G < 0) G = 0;
            else if (G > 255) G = 255;
            if (B < 0) B = 0;
            else if (B > 255) B = 255;

            *out_ptr++ = (unsigned char)R;
            *out_ptr++ = (unsigned char)G;
            *out_ptr++ = (unsigned char)B;
            *out_ptr++ = 255;
        }
    }
}

bool isRunning = true;

void camera_update()
{
	while (isRunning){
		if (!ps4eye::PS4EYECam::updateDevices())
			break;
	}
}

void initialize() {
	cout << "init" << endl;
	glGenTextures(1, &texture_left);
	glGenTextures(1, &texture_right);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void drawScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture_left);
	glBegin(GL_QUADS);
	    glTexCoord2f(0.0, 0.0); glVertex2f(0, -height);
	    glTexCoord2f(1.0, 0.0); glVertex2f(-width, -height);
	    glTexCoord2f(1.0, 1.0); glVertex2f(-width, height);
	    glTexCoord2f(0.0, 1.0); glVertex2f(0, height);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, texture_right);
	glBegin(GL_QUADS);
	    glTexCoord2f(0.0, 0.0); glVertex2f(width, -height);
	    glTexCoord2f(1.0, 0.0); glVertex2f(0, -height);
	    glTexCoord2f(1.0, 1.0); glVertex2f(0, height);
	    glTexCoord2f(0.0, 1.0); glVertex2f(width, height);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glFlush();
	// glutSwapBuffers();
}

void resizeWindow(int w, int h)
{
	float scale = float(w)/(2.0*width);

	int padding = ((float)h - height*scale)/2.0;

	glViewport(0, padding, w, h - 2*padding);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-width, width, -height, height, -1.0, 1.0);

	// gluOrtho2D(0, width, height, 0)

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void keyboard (unsigned char key, int x, int y)
{
	cout << "key: " << (int)key << endl;
	switch (key) {
	case 27:
		isRunning = false;
		// update_thread.join();
		camera->stop();
		camera->shutdown();
		cout << "Exiting" << endl;
		exit(0);
		break;
	case 'f':
		glutFullScreen();
		break;
	default:
		break;
	}
}

// std::chrono::high_resolution_clock::time_point start;

void catch_video_frame(){
	if (camera->isNewFrame()){
		// auto elapsed = std::chrono::high_resolution_clock::now() - start;
		// long long num = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		// cout << num << " = " << 1000000.0 / num << endl;
		eyeframe *frame = camera->getLastVideoFramePointer();
		convert_YUYV_to_RGBA8(frame->videoLeftFrame, frame_rgb_left, width, height);
		convert_YUYV_to_RGBA8(frame->videoRightFrame, frame_rgb_right, width, height);

		glBindTexture(GL_TEXTURE_2D, texture_left);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_rgb_left);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		
		glBindTexture(GL_TEXTURE_2D, texture_right);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame_rgb_right);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		// start = std::chrono::high_resolution_clock::now();
	}
	glutPostRedisplay();
}

int main(int argc, char** argv){
	std::vector<ps4eye::PS4EYECam::PS4EYERef> devices( ps4eye::PS4EYECam::getDevices() );
	if (devices.size() == 0)
	{
		cout << "camera not found" << std::endl;
		return 1;
	}
	camera = devices.at(0);
	camera->firmware_path = "firmware.bin";
	if (camera->firmware_upload())
	{
		return 0;
		ps4eye::PS4EYECam::updateDevices();
		camera = devices.at(0);
	}

	if (!camera->init(1, 120))
	{	
		cout << "init failed" << std::endl;
		// exit(0);
	}

	camera->start();

	height = camera->getHeight();
	width = camera->getWidth();
	cout << width << "x" << height << endl;
	frame_rgb_size = width * height * 4;
	frame_rgb_left = new uint8_t[frame_rgb_size];
	frame_rgb_right = new uint8_t[frame_rgb_size];
	memset(frame_rgb_left, 0, frame_rgb_size);
    memset(frame_rgb_right, 0, frame_rgb_size);

    // videoFrame 	= new unsigned char[frame_rgb_size];

	std::thread update_thread(camera_update);
	// std::thread grab_thread(catch_video_frame);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(width, height/2);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Window!");
	initialize();
	glutDisplayFunc(drawScene);
	glutReshapeFunc(resizeWindow);
	glutKeyboardFunc(keyboard);

	glutIdleFunc(catch_video_frame);

	glutMainLoop();
	return 0;
}

