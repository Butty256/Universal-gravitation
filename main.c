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
#define DT 1.0
#define GC 6.674e-11

/********************************
	構造体
********************************/

struct materialStruct
{
	GLfloat ambient[4], diffuse[4], specular[4];
	GLfloat shininess;
};
typedef struct materialStruct materialStruct;

struct Object
{
	GLdouble m;
	GLdouble p[3];
	GLdouble v[3];
	GLdouble r;
	materialStruct c;
};
typedef struct Object Object;

/********************************
	関数のプロトタイプ宣言
********************************/

void display(void);
void materials(materialStruct m);
void saveImage(void);
static void timer(int dummy);
void resize(int w, int h);
void keyin(unsigned char key, int x, int y);
void motionActive(int x, int y);
void motionPassive(int x, int y);
void mouse(int button, int state, int x, int y);
void readcsv(FILE *fp);
void init(void);

/********************************
	グローバル変数
********************************/

/* 物体 */
Object g_obj[DATANUM];
int g_onum;

/* Quadric */
GLUquadric *g_quad;

/* 単位スケール */
GLdouble g_s = 1, g_m = 1, g_kg = 1;

/* カメラの視点e，カメラの上方向u */
GLdouble g_ex, g_ey, g_ez;
GLdouble g_ux = 0.0, g_uy = 0.0, g_uz = 1.0;

/* 描画サイズ */
GLdouble g_size = 10.0;

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
	gluLookAt(g_ex, g_ey, g_ez, 0, 0, 0, g_ux, g_uy, (sin(g_mt) > 0) ? g_uz : -g_uz);

	for (i = 0; i < g_onum; i++)
	{
		glTranslatef(g_obj[i].p[0], g_obj[i].p[1], g_obj[i].p[2]);
		glCallList(i + 1);
		glTranslatef(-g_obj[i].p[0], -g_obj[i].p[1], -g_obj[i].p[2]);
	}

	glutSwapBuffers();
}

/* マテリアル */
void materials(materialStruct m)
{
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m.specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m.shininess);
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
	glutTimerFunc(DT, timer, 0);

	for (i = 0; i < g_onum; i++)
	{
		g_obj[i].p[0] += g_obj[i].v[0] * DT / 1000 * g_s;
		g_obj[i].p[1] += g_obj[i].v[1] * DT / 1000 * g_s;
		g_obj[i].p[2] += g_obj[i].v[2] * DT / 1000 * g_s;
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
			if (r <= g_obj[i].r + g_obj[j].r) continue;
			g_obj[i].v[0] += GC * g_obj[j].m / (r * r) * (x / r) * DT / 1000 * g_kg / (g_m * g_m) * g_s / g_m;
			g_obj[i].v[1] += GC * g_obj[j].m / (r * r) * (y / r) * DT / 1000 * g_kg / (g_m * g_m) * g_s / g_m;
			g_obj[i].v[2] += GC * g_obj[j].m / (r * r) * (z / r) * DT / 1000 * g_kg / (g_m * g_m) * g_s / g_m;
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
	glOrtho(-w / g_size, w / g_size, -h / g_size, h / g_size, -32768, 32767);
}

/* キーボードコールバック関数 */
void keyin(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 't': g_s *= 2; glutPostRedisplay(); break;
		case 'T': g_s /= 2; glutPostRedisplay(); break;
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
	if (sin(g_mt_s) < 0) dx *= -1;

	g_mp = g_mp_s - 0.001 * dx;
	g_mt = g_mt_s - 0.001 * dy;

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

/* csv読み込み関数 */
void readcsv(FILE *fp)
{
	GLfloat r, g, b;
	char buf[128];
	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		if (buf[0] == '#') continue;
		if (buf[0] == 's' || buf[0] == 'S')
		{
			strtok(buf, ",\0");
			g_s = atof(strtok(NULL, ",\0"));
			g_m = atof(strtok(NULL, ",\0"));
			g_kg = atof(strtok(NULL, "\0"));
			continue;
		}
		g_obj[g_onum].m = atof(strtok(buf, ",\0"));
		g_obj[g_onum].p[0] = atof(strtok(NULL, ",\0"));
		g_obj[g_onum].p[1] = atof(strtok(NULL, ",\0"));
		g_obj[g_onum].p[2] = atof(strtok(NULL, ",\0"));
		g_obj[g_onum].v[0] = atof(strtok(NULL, ",\0"));
		g_obj[g_onum].v[1] = atof(strtok(NULL, ",\0"));
		g_obj[g_onum].v[2] = atof(strtok(NULL, ",\0"));
		g_obj[g_onum].r = atof(strtok(NULL, ",\0"));
		r = atof(strtok(NULL, ",\0"));
		g = atof(strtok(NULL, ",\0"));
		b = atof(strtok(NULL, "\0"));
		g_obj[g_onum].c.ambient[0] = 0.3 * r;
		g_obj[g_onum].c.ambient[1] = 0.3 * g;
		g_obj[g_onum].c.ambient[2] = 0.3 * b;
		g_obj[g_onum].c.ambient[3] = 1;
		g_obj[g_onum].c.diffuse[0] = 0.6 * r;
		g_obj[g_onum].c.diffuse[1] = 0.6 * g;
		g_obj[g_onum].c.diffuse[2] = 0.6 * b;
		g_obj[g_onum].c.diffuse[3] = 1;
		g_obj[g_onum].c.specular[0] = 0.2 * r + 0.6;
		g_obj[g_onum].c.specular[1] = 0.2 * g + 0.6;
		g_obj[g_onum].c.specular[2] = 0.2 * b + 0.6;
		g_obj[g_onum].c.specular[3] = 1;
		g_obj[g_onum].c.shininess = 32;
		g_onum++;
		if (g_onum >= DATANUM) break;
	}
}

/* 初期化する関数 */
void init(void)
{
	int i;

	g_ex = sin(g_mt) * cos(g_mp);
	g_ey = sin(g_mt) * sin(g_mp);
	g_ez = cos(g_mt);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	g_quad = gluNewQuadric();
	gluQuadricDrawStyle(g_quad, GLU_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	for (i = 0; i < g_onum; i++)
	{
		glNewList(i + 1, GL_COMPILE);
		glPushMatrix();
		materials(g_obj[i].c);
		gluSphere(g_quad, g_obj[i].r, 16, 16);
		glPopMatrix();
		glEndList();
	}
}

/* メイン関数 */
int main(int argc, char **argv)
{
	FILE *fp;
	char filename[16] = "default.csv";

	if (argc == 2) strcpy(filename, argv[1]);
	if ((fp = fopen(filename, "rt")) == NULL)
	{
		fprintf(stderr, "%s is not found.\n", filename);
		return 1;
	}
	readcsv(fp);
	fclose(fp);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(700, 700);
	glutCreateWindow(argv[0]);
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutTimerFunc(DT, timer, 0);
	glutKeyboardFunc(keyin);
	glutMotionFunc(motionActive);
	glutPassiveMotionFunc(motionPassive);
	glutMouseFunc(mouse);
	init();
	glutMainLoop();

	return EXIT_SUCCESS;
}
