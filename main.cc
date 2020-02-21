#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "Graph.h"
#include "Point.h"
#include "Graphs/GL_Util.h"
static Graph graph;

static void reshape(int w, int h)
{
	graph.viewport(w, h);
}

static void key(unsigned char c, int x, int y)
{
	Graph &g = graph;
	
	switch (c)
	{
		case 'q':
		case 'Q':
		case 27: g.record(false); exit(0);

		case 'r':
			g.record(!g.recording());
			g.animate(g.recording());
			break;

		case ' ':
			g.animate(!g.animating());
			break;

		case 8: // Backspace
			g.reset();
			glutPostRedisplay();
			break;

		case 'n':
			glutPostRedisplay();
			break;

		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '0':
		{
			int k = (c == '0' ? 10 : c - '0');
			g.timezoom(k);
			g.animate(true);
			glutPostRedisplay();
			break;
		}

		case 'v':
			++Point::vis;
			glutPostRedisplay();
			break;
		case 'V':
			--Point::vis;
			glutPostRedisplay();
			break;

		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h':
			Point::vis = c - 'a';
			glutPostRedisplay();
			break;
	}
}

static void draw()
{
	GL_CHECK;
	graph.draw();
	GL_CHECK;

	//glFinish();
	glFlush();
	glutSwapBuffers();
}

static void idle()
{
	#if 0
	static double t0 = -1.;
	double t = glutGet(GLUT_ELAPSED_TIME) / 1000.;
	if (t0 < 0.) t0 = t;
	double dt;
	dt = t - t0;
	t0 = t;
	#endif

	if (graph.animating()) glutPostRedisplay();
}

static void visible(int vis)
{
	glutIdleFunc(vis == GLUT_VISIBLE ? idle : NULL);
}

int main(int argc, char *argv[])
{
	//graph.animate(true);
	glutInitWindowSize(600, 600);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	glutCreateWindow("WPlot");
	glDisable(GL_DITHER);
	glShadeModel(GL_FLAT);

	glutDisplayFunc(draw);
	glutReshapeFunc(reshape);
	glutVisibilityFunc(visible);
	glutKeyboardFunc(key);

	glutMainLoop();
	return 0;
}

