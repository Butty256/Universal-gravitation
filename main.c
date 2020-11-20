#include "wbmp.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

/********************************
	マクロ定義
********************************/

#define DATANUM 16
#define GC 6.674e-11

/********************************
	関数のプロトタイプ宣言
********************************/

void display(void);
void saveImage(void);
static void timer(int dummy);
void resize(int w, int h);
void keyin(unsigned char key, int x, int y);
void motionActive(int x, int y);
void motionPassive(int x, int y);
void mouse(int button, int state, int x, int y);
void init(void);

/********************************
	構造体
********************************/

struct Object
{
	GLUquadric *o;
	GLdouble m;
	GLdouble p[3];
	GLdouble v[3];
	GLdouble r;
	GLfloat c[3];
};
typedef struct Object Object;

/********************************
	グローバル変数
********************************/

/* 物体 */
Object g_obj[DATANUM];
int g_onum;

/* 万有引力定数 */
GLdouble g_gc = GC;

/* 時間 */
GLdouble g_dt = 50;

/* カメラの視点e，注視点a，カメラの上方向u */
GLdouble g_ex, g_ey, g_ez;
GLdouble g_ax = 0.0, g_ay = 0.0, g_az = 0.0;
GLdouble g_ux = 0.0, g_uy = 0.0, g_uz = 1.0;

/* 描画サイズ */
GLdouble g_size = 700.0;
GLdouble g_xscale = 0.1, g_yscale = 0.1, g_zscale = 0.1;

/* マウスの角度 */
int g_mx, g_my;
double g_mt_s, g_mp_s, g_mt = M_PI / 4, g_mp = 3 * M_PI / 4;

/********************************
	関数の定義
********************************/

/* 描画コールバック関数 */
void display(void)
{
	int i;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		g_ex + (g_xscale * g_ax), g_ey + (g_yscale * g_ay), g_ez + (g_zscale * g_az),
		g_xscale * g_ax, g_yscale * g_ay, g_zscale * g_az,
		g_ux, g_uy, g_uz
	);
	glScaled(g_xscale, g_yscale, g_zscale);

	for (i = 0; i < g_onum; i++)
	{
		glTranslatef(g_obj[i].p[0], g_obj[i].p[1], g_obj[i].p[2]);
		glCallList(i + 1);
		glTranslatef(-g_obj[i].p[0], -g_obj[i].p[1], -g_obj[i].p[2]);
	}

	glutSwapBuffers();
}

/* 画像保存関数 */
void saveImage(void)
{
	GLubyte *buf;
	Image *img;
	char filename[64];
	int width, height, i, j;

	width = glutGet(GLUT_WINDOW_WIDTH);
	height = glutGet(GLUT_WINDOW_HEIGHT);

	buf = (GLubyte*)malloc(width * height * 3);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width , height, GL_RGB, GL_UNSIGNED_BYTE, buf);

	if ((img = createImage(width, height)) == NULL)
	{
		fprintf(stdout, "cannot create image\n");
		return;
	};
	for (i = 0; i < img->height; i++)
	{
		for (j = 0; j < img->width; j++)
		{
			img->data[(img->height - i - 1) * img->width + j].r = buf[3 * (width * i + j)];
			img->data[(img->height - i - 1) * img->width + j].g = buf[3 * (width * i + j) + 1];
			img->data[(img->height - i - 1) * img->width + j].b = buf[3 * (width * i + j) + 2];
		}
	}
	sprintf(filename, "%ld.bmp", time(NULL));
	if (writeBmp(img, filename))
	{
		fprintf(stdout, "cannot write image\n");
		freeImage(img);
		return;
	};
	freeImage(img);
	fprintf(stdout, "saved %s\n", filename);
}

/* タイマーコールバック関数 */
static void timer(int dummy)
{
	int i, j;
	GLdouble x, y, z, r;
	glutTimerFunc(g_dt, timer, 0);

	for (i = 0; i < g_onum; i++)
	{
		g_obj[i].p[0] += g_obj[i].v[0] * (g_dt / 1000);
		g_obj[i].p[1] += g_obj[i].v[1] * (g_dt / 1000);
		g_obj[i].p[2] += g_obj[i].v[2] * (g_dt / 1000);
	}

	for (i = 0; i < g_onum; i++)
	{
		for (j = 0; j < g_onum; j++)
		{
			if (i == j) continue;
			x = g_obj[j].p[0] - g_obj[i].p[0];
			y = g_obj[j].p[1] - g_obj[i].p[1];
			z = g_obj[j].p[2] - g_obj[i].p[2];
			r = sqrt(x * x + y * y + z * z);
			if (r >= g_obj[i].r + g_obj[j].r)
			{
				g_obj[i].v[0] += g_gc * g_obj[j].m / (r * r) * (x / r) * (g_dt / 1000);
				g_obj[i].v[1] += g_gc * g_obj[j].m / (r * r) * (y / r) * (g_dt / 1000);
				g_obj[i].v[2] += g_gc * g_obj[j].m / (r * r) * (z / r) * (g_dt / 1000);
			}
			//else
			//{
			//	g_obj[i].v[0] *= -1;
			//	g_obj[i].v[1] *= -1;
			//	g_obj[i].v[2] *= -1;
			//}
		}
	}

	glutPostRedisplay();
}

/* リサイズコールバック関数 */
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-w / g_size, w / g_size, -h / g_size, h / g_size, -1024, 1024);
}

/* キーボードコールバック関数 */
void keyin(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 'x': g_xscale *= 1.2; glutPostRedisplay(); break;
		case 'X': g_xscale /= 1.2; glutPostRedisplay(); break;
		case 'y': g_yscale *= 1.2; glutPostRedisplay(); break;
		case 'Y': g_yscale /= 1.2; glutPostRedisplay(); break;
		case 'z': g_zscale *= 1.2; glutPostRedisplay(); break;
		case 'Z': g_zscale /= 1.2; glutPostRedisplay(); break;
		case 'g': g_gc *= 10; glutPostRedisplay(); break;
		case 'G': g_gc /= 10; glutPostRedisplay(); break;
		case 't': g_dt *= 1.2; glutPostRedisplay(); break;
		case 'T': g_dt /= 1.2; glutPostRedisplay(); break;
		case 'i': g_dt *= -1; glutPostRedisplay(); break;
		case 'u': g_size *= 1.05; resize(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT)); glutPostRedisplay(); break;
		case 'U': g_size /= 1.05; resize(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT)); glutPostRedisplay(); break;
		case 'S': saveImage(); break;
		case '\033': exit(0);
		default: break;
	}
}

/* マウスのボタンが押されているときの処理 */
void motionActive(int x, int y)
{
	int dx, dy;

	dx = x - g_mx;
	dy = y - g_my;

	g_mp = g_mp_s - 0.001 * dx;
	g_mt = g_mt_s - 0.001 * dy;

	if (g_mt < 0) g_mt += M_PI, g_mp += M_PI;
	else if (g_mt > M_PI) g_mt -= M_PI, g_mp -= M_PI;

	g_ex = sin(g_mt) * cos(g_mp);
	g_ey = sin(g_mt) * sin(g_mp);
	g_ez = cos(g_mt);

	glutPostRedisplay();
}

/* マウスのボタンが押されていないときの処理 */
void motionPassive(int x, int y)
{
	g_mx = x;
	g_my = y;
	g_mt_s = g_mt;
	g_mp_s = g_mp;
}

/* マウスのホイールの処理 */
void mouse(int button, int state, int x, int y)
{
	if (button == 3)
	{
		g_size *= 1.05;
		resize(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		glutPostRedisplay();
	}
	else if (button == 4)
	{
		g_size /= 1.05;
		resize(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		glutPostRedisplay();
	}
}

/* 初期化する関数 */
void init(void)
{
	int i;
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_DEPTH_TEST);

	g_ex = sin(g_mt) * cos(g_mp);
	g_ey = sin(g_mt) * sin(g_mp);
	g_ez = cos(g_mt);

	for (i = 0; i < g_onum; i++)
	{
		g_obj[i].o = gluNewQuadric();
		//gluQuadricDrawStyle(g_obj[i].o, GLU_LINE);
		glNewList(i + 1, GL_COMPILE);
		glColor3fv(g_obj[i].c);
		glPushMatrix();
		gluSphere(g_obj[i].o, g_obj[i].r, 16, 16);
		glPopMatrix();
		glEndList();
	}
}

/* メイン関数 */
int main(int argc, char **argv)
{
	char buf[64];

	fprintf(stdout, "m px py pz vx vy vz r R G B\nend command: 0\n");

	for (g_onum = 0; g_onum < DATANUM; g_onum++)
	{
		fgets(buf, sizeof(buf), stdin);
		g_obj[g_onum].m = atof(strtok(buf, " \0"));
		if (g_obj[g_onum].m == 0) break;
		g_obj[g_onum].p[0] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].p[1] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].p[2] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].v[0] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].v[1] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].v[2] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].r = atof(strtok(NULL, " \0"));
		g_obj[g_onum].c[0] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].c[1] = atof(strtok(NULL, " \0"));
		g_obj[g_onum].c[2] = atof(strtok(NULL, " \0"));
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(700, 700);
	glutCreateWindow(argv[1]);
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutTimerFunc(g_dt, timer, 0);
	glutKeyboardFunc(keyin);
	glutMotionFunc(motionActive);
	glutPassiveMotionFunc(motionPassive);
	glutMouseFunc(mouse);

	init();
	glutMainLoop();

	return EXIT_SUCCESS;
}
