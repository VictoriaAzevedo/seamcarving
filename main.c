#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings

#ifdef WIN32
#include <windows.h> // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>   // Funções da OpenGL
#include <GL/glu.h>  // Funções da GLU
#include <GL/glut.h> // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include <SOIL.h>

#include <stdbool.h>
#include <limits.h>

#define MIN2(x,y) ((x<y)?x:y)
#define MIN3(x,y,z) (MIN2(MIN2(x,y),z))
#define MAX2(x,y) ((x>y)?x:y)
#define MAX3(x,y,z) (MAX2(MAX2(x,y),z))
// Um pixel RGB (24 bits)
typedef struct
{
    unsigned char r, g, b;
} RGB8;

// Uma imagem RGB
typedef struct
{
    int width, height;
    RGB8 *img;
} Img;

// Protótipos
void load(char *name, Img *pic);
void uploadTexture();
void seamcarve(int targetWidth); // executa o algoritmo
void freemem();                  // limpa memória (caso tenha alocado dinamicamente)

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);

//Funções adicionadas
void copiaImagemOriginal();
void calculoEnergiaPixels(int* matrizPesos);
void aplicacaoMascara(int* matrizPesos);
bool pixelVermelho(RGB8 pixel);
void matrizCustoAcumulado(int* matrizPesos);
void listaSeam(int* matrizPesos, int* seam);
void remocaoSeam(int* seam);

// Largura e altura da janela
int width, height;

// Largura desejada (selecionável)
int targetW;

//Largura atual
int currentTargetWidth;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];
Img *source;
Img *mask;
Img *target;

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char *name, Img *pic)
{
    int chan;
    pic->img = (RGB8 *)SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if (!pic->img)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

void copiaImagemOriginal() {
    RGB8 (*sourcePtr)[source->width] = (RGB8(*)[source->width])source->img;
    RGB8 (*targetPtr)[width] = (RGB8(*)[width]) target->img;

    for (int i = 0; i < source->height; i++) {
        for (int j = 0; j < source->width; j++) {
            targetPtr[i][j].r = sourcePtr[i][j].r;
            targetPtr[i][j].g = sourcePtr[i][j].g;
            targetPtr[i][j].b = sourcePtr[i][j].b;
        }
    }
}

void copiaMascara(RGB8* copia) {
    RGB8 (*maskPtr)[source->width] = (RGB8(*)[source->width])mask->img;
    RGB8 (*copiaPtr)[width] = (RGB8(*)[width]) copia;

    for (int i = 0; i < source->height; i++) {
        for (int j = 0; j < source->width; j++) {
            copiaPtr[i][j] = maskPtr[i][j];
        }
    }
}

void calculoEnergiaPixels(int* matrizPesos){
    int rx, gx, bx;
    int ry, gy, by;
    int gradX, gradY, grad;
    int targetWidth = target->width;
    RGB8(*targetPtr)[targetWidth] = (RGB8(*)[targetWidth])target->img;
    int (*matrizPesosPtr)[targetWidth] = (int(*)[targetWidth]) matrizPesos;

    for (int i = 0; i < target->height; i++){
        for (int j = 0; j < targetWidth; j++){
            
            //Energia em X
            if (j == 0) {// pixels a esquerda
                rx = targetPtr[i][j+1].r - targetPtr[i][targetWidth - 1].r;
                gx = targetPtr[i][j+1].g - targetPtr[i][targetWidth - 1].g;
                bx = targetPtr[i][j+1].b - targetPtr[i][targetWidth - 1].b;
            } else if (j == targetWidth - 1) { //pixels a direita
                rx = targetPtr[i][0].r - targetPtr[i][j - 1].r;
                gx = targetPtr[i][0].g - targetPtr[i][j - 1].g;
                bx = targetPtr[i][0].b - targetPtr[i][j - 1].b;
            } else { //pixels no centro
                rx = targetPtr[i][j+1].r - targetPtr[i][j - 1].r;
                gx = targetPtr[i][j+1].g - targetPtr[i][j - 1].g;
                bx = targetPtr[i][j+1].b - targetPtr[i][j - 1].b;
            }


            //Energia em Y
            if (i == 0) {// pixels no topo
                ry = targetPtr[height - 1][j].r - targetPtr[i+1][j].r;
                gy = targetPtr[height - 1][j].g - targetPtr[i+1][j].g;
                by = targetPtr[height - 1][j].b - targetPtr[i+1][j].b;
            } else if (i == height - 1) { //pixels na base
                ry = targetPtr[i - 1][j].r - targetPtr[0][j].r;
                gy = targetPtr[i - 1][j].g - targetPtr[0][j].g;
                by = targetPtr[i - 1][j].b - targetPtr[0][j].b;
            } else { //pixels no centro
                ry = targetPtr[i + 1][j].r - targetPtr[i - 1][j].r;
                gy = targetPtr[i + 1][j].g - targetPtr[i - 1][j].g;
                by = targetPtr[i + 1][j].b - targetPtr[i - 1][j].b;
            }
        
        //Gradiente em X
        gradX = rx*rx + gx*gx + bx*bx;

        //Gradiente em Y
        gradY = ry*ry + gy*gy + by*by;

        //Gradiente
        grad = gradX + gradY;
        matrizPesosPtr[i][j] = grad;
        }
    }
}

bool pixelVermelho(RGB8 pixel) {
    int green = (int) pixel.g;
    if(green <  200) {
       return true;
    }

    return false;
}

bool pixelVerde(RGB8 pixel) {
    int red = (int) pixel.r;
    if(red <  200) {
       return true;
    }
    return false;
}

void aplicacaoMascara(int* matrizPesos) {
    RGB8 (*mascaraPtr)[mask->width] = (RGB8(*)[mask->width]) mask->img;
    int (*matrizPesosPtr)[target->width] = (int(*)[target->width]) matrizPesos;
    for(int i = 0; i < target->height; i++) {
        for(int j = 0; j < target->width; j++) {
            RGB8 pixel = mascaraPtr[i][j];
            if(pixelVermelho(pixel)) {
                matrizPesosPtr[i][j] = INT_MIN;
            } else if (pixelVerde(pixel)) {
                matrizPesosPtr[i][j] = INT_MAX;
            }
        }
    }
}

void matrizCustoAcumulado(int* matrizPesos) {
    //Cálculo da Matriz de Custo acumulado
    int (*matrizPesosPtr)[target->width] = (int(*)[target->width]) matrizPesos;
    int primeiroPixel;
    int segundoPixel;
    int terceiroPixel;
    int min;
    for (int i = 0; i < target->height; i++) {
        for (int j = 0; j < target->width; j++) {
            if (j == 0) { //Canto esquerdo
                primeiroPixel = matrizPesosPtr[i][j];
                segundoPixel = matrizPesosPtr[i][j+1];
                min = MIN2(primeiroPixel , segundoPixel);
                matrizPesosPtr[i+1][j] = matrizPesosPtr[i+1][j] + min;

            } else if (j == target->width - 1) { //Canto direito
                primeiroPixel = matrizPesosPtr[i][j];
                segundoPixel = matrizPesosPtr[i][j-1];
                min = MIN2(primeiroPixel, segundoPixel);
                matrizPesosPtr[i+1][j] = matrizPesosPtr[i+1][j] + min;

            } else { //Centro da imagem
                primeiroPixel = matrizPesosPtr[i][j-1];
                segundoPixel = matrizPesosPtr[i][j];
                terceiroPixel = matrizPesosPtr[i][j+1];
                min = MIN3(primeiroPixel, segundoPixel, terceiroPixel);
                matrizPesosPtr[i+1][j] = matrizPesosPtr[i+1][j];
            }
        }
    }
}

void listaSeam(int* matrizPesos, int* seam) {
    //Identificar o melhor caminho para o seamcarving
    int (*matrizPesosPtr)[target->width] = (int(*)[target->width]) matrizPesos;
    int primeiroPixel;
    int segundoPixel;
    int terceiroPixel;
    int minSeam = matrizPesosPtr[height - 1][0];
    int posicaoPixel = 0;
    //Encontrar o pixel com menor acumulo na base da matriz
    for(int j = 0; j < width - 1; j++) {
        int segundoValor = matrizPesosPtr[height - 1][j];
        minSeam = MIN2(minSeam, segundoValor);
        if(minSeam == segundoValor) {
            posicaoPixel = j;
        }    
    }
    seam[height - 1] = posicaoPixel;
    
    //Encontrar próximos pixels
    for (int i = height - 2; 0 <= i; i--) {
        int j = seam[i + 1];
        //Se pixel de baixo é no canto esquerdo
        if(posicaoPixel == 0){
            primeiroPixel = matrizPesosPtr[i][j];
            segundoPixel = matrizPesosPtr[i][j+1];
            minSeam = MIN2(primeiroPixel, segundoPixel);
            if (minSeam == primeiroPixel){
                posicaoPixel = j;
            } else {
                posicaoPixel = j + 1;
            }

        //Se pixel de baixo é no canto direito
        } else if (posicaoPixel == target->width) {
            primeiroPixel = matrizPesosPtr[i][j-1];
            segundoPixel = matrizPesosPtr[i][j];
            minSeam = MIN2(primeiroPixel, segundoPixel);
            if (minSeam == primeiroPixel){
                posicaoPixel = j;
            } else {
                posicaoPixel = j + 1;
            }
        //Se pixel de baixo é no meio
        } else {
            primeiroPixel = matrizPesosPtr[i][j-1];
            segundoPixel = matrizPesosPtr[i][j];
            terceiroPixel = matrizPesosPtr[i][j+1];
            minSeam = MIN3(primeiroPixel ,segundoPixel, terceiroPixel);
            if (minSeam == primeiroPixel){
                posicaoPixel = j - 1;
            } else if (minSeam == segundoPixel) {
                posicaoPixel = j;
            } else {
                posicaoPixel = j + 1;
            }
        }
        seam[i] = posicaoPixel;
    }

}

void remocaoSeam(int* seam) {
    int numeroPixel;
    int numeroPixelsRemovidos = 0;
    RGB8* imagem = malloc((target->width - 1)* target->height * 3);
    for (int i = 0; i < target->height; i++){
        for(int j = 0; j < target->width - 1; j++) {
            if(j == seam[i]) {
                numeroPixelsRemovidos++;
            }
            imagem[numeroPixel] = target->img[numeroPixel + numeroPixelsRemovidos];
        }
    }
    target->width = target->width - 1;
    free(pic[2].img);
    pic[2].img = imagem;

}

void remocaoSeamMascara(int* seam) {
    int numeroPixel;
    int numeroPixelsRemovidos = 0;
    RGB8* copia = malloc((mask->width) * mask->height * 3);
    for (int i = 0; i < mask->height; i++){
        for(int j = 0; j < mask->width - 1; j++) {
            if(j == seam[i]) {
                numeroPixelsRemovidos++;
            }
            copia[numeroPixel] = mask->img[numeroPixel + numeroPixelsRemovidos];
        }
    }
    mask->width = mask->width - 1;
    free(pic[1].img);
    pic[1].img = copia;

}


//
// Implemente AQUI o seu algoritmo
void seamcarve(int targetWidth)
{ 
    int seam[height];
    if(target->width == source->width){
        copiaImagemOriginal();
    }

    while (target->width > targetW) {
        
        int *matrizPesos = malloc(target->height * target->width * sizeof(int));
        calculoEnergiaPixels(matrizPesos);
        aplicacaoMascara(matrizPesos);
        matrizCustoAcumulado(matrizPesos);
        listaSeam(matrizPesos, &seam[0]);
        remocaoSeam(&seam[0]);
        remocaoSeamMascara(&seam[0]);
        free(matrizPesos);
    }

    // Aplica o algoritmo e gera a saida em target->img...
    /*RGB8(*ptr)[target->width] = (RGB8(*)[target->width])target->img;

    for (int y = 0; y < target->height; y++)
    {
        for (int x = 0; x < targetW; x++)
            ptr[y][x].r = ptr[y][x].g = 255;
        for (int x = targetW; x < target->width; x++)
            ptr[y][x].r = ptr[y][x].g = 0;
    }*/
    // Chame uploadTexture a cada vez que mudar
    // a imagem (pic[2])
    uploadTexture();
    glutPostRedisplay();
}

void freemem()
{
    // Libera a memória ocupada pelas 3 imagens
    free(pic[0].img);
    free(pic[1].img);
    free(pic[2].img);
}

/********************************************************************
 * 
 *  VOCÊ NÃO DEVE ALTERAR NADA NO PROGRAMA A PARTIR DESTE PONTO!
 *
 ********************************************************************/
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc, argv);
printf("Origem é a imagem original, mascara é a máscara desejada\n");
    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if (pic[0].width != pic[1].width || pic[0].height != pic[1].height)
    {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width = pic[1].width;
    pic[2].height = pic[1].height;

    // Ponteiros para as structs das imagens, para facilitar
    source = &pic[0];
    mask = &pic[1];
    target = &pic[2];

    // Largura desejada inicialmente é a largura da janela
    targetW = target->width;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(keyboard);

    // Registra a funcao callback para tratamento das setas
    glutSpecialFunc(arrow_keys);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char *)pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char *)pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Máscara : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width * height * 3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char *)pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // ESC: libera memória e finaliza
        freemem();
        exit(1);
    }
    if (key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if (key == 's')
    {
        seamcarve(targetW);
    }
    glutPostRedisplay();
}

void arrow_keys(int a_keys, int x, int y)
{
    switch (a_keys)
    {
    case GLUT_KEY_RIGHT:
        if (targetW <= pic[2].width - 10)
            targetW += 10;
        seamcarve(targetW);
        break;
    case GLUT_KEY_LEFT:
        if (targetW > 10)
            targetW -= 10;
        seamcarve(targetW);
        break;
    default:
        break;
    }
}
// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 target->width, target->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, target->img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Preto
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255); // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);

    glTexCoord2f(1, 0);
    glVertex2f(pic[sel].width, 0);

    glTexCoord2f(1, 1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0, 1);
    glVertex2f(0, pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
