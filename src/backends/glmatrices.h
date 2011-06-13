#define LSGL_MATRIX_SIZE (16*sizeof(GLfloat))

extern GLfloat lsIdentityMatrix[16];
extern GLfloat lsMVPMatrix[16];
void lsglLoadMatrixf(const GLfloat *m);
void lsglLoadIdentity();
void lsglMultMatrixf(const GLfloat *m);
void lsglScalef(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
void lsglTranslatef(GLfloat translateX, GLfloat translateY, GLfloat translateZ);
void lsglOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void setMatrixUniform(bool mv);
void lsglPushMatrix();
void lsglPopMatrix();
