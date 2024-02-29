#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

double height, width;
double** matrix;
double scale = 1.4;
double rotateX = 0.0;
double rotateY = 0.0;
double scaleFactor = 1;

glm::vec3 viewDir (0.0, 0.0, 0.0);
glm::vec3 eyePos(0.0, 0.0, 1000.0);

int model = 0;

void WriteToFile(const char* filename, int width, int height) {
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Ошибка записи" << std::endl;
        return;
    }

    const int headersize = 54;
    char bmpfileheader[54] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, headersize, 0, 0, 0, 40, 0, 0, 0, (char)(width & 0x000000FF), (char)((width & 0x0000FF00) >> 8), (char)((width & 0x00FF0000) >> 16), (char)((width & 0xFF000000) >> 24), (char)(height & 0x000000FF), (char)((height & 0x0000FF00) >> 8), (char)((height & 0x00FF0000) >> 16), (char)((height & 0xFF000000) >> 24), 1, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    file.write(bmpfileheader, headersize);

    char* pixels = new char[width * height * 3];

    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels);

    for (int i = 0; i < width * height * 3; i += 3) {
        char temp = pixels[i];
        pixels[i] = pixels[i + 2];
        pixels[i + 2] = temp;
    }

    file.write(pixels, width * height * 3);

    file.close();
    delete[] pixels;

    std::cout << "Картинка сохранена в " << filename << std::endl;
}

void ReadFromFile() {

    std::ifstream file("DepthMap_1.dat", std::ios::binary);

    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&height), sizeof(double));
        file.read(reinterpret_cast<char*>(&width), sizeof(double));

        matrix = new double* [static_cast<int>(height)];

        for (int i = 0; i < static_cast<int>(height); i++) {
            matrix[i] = new double[static_cast<int>(width)];

            for (int j = 0; j < static_cast<int>(width); j++) {
                file.read(reinterpret_cast<char*>(&matrix[i][j]), sizeof(double));
            }
        }
        file.close();
    }
}

void Lambert(int i, int j) {
    glm::vec3 v1(i, j, matrix[i][j]);
    glm::vec3 v2(i - 1, j, matrix[i - 1][j]);
    glm::vec3 v3(i - 1, j + 1, matrix[i - 1][j + 1]);
    glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
    normal = glm::normalize(normal);
    glm::vec3 lightDir = glm::normalize(glm::vec3(viewDir.x, viewDir.y, viewDir.z) - v1);
    float kd = 1.0f; // коэффициент диффузного отражения

    float diff = kd * std::max(glm::dot(normal, lightDir), 0.0f);

    glm::vec3 color(1.0f, 1.0f, 1.0f);
    glColor3f(color.r * diff, color.g * diff, color.b * diff);
}

void Fong(int i, int j) {
    glm::vec3 v1(i, j, matrix[i][j]);
    glm::vec3 v2(i - 1, j, matrix[i - 1][j]);
    glm::vec3 v3(i - 1, j + 1, matrix[i - 1][j + 1]);
    glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
    normal = glm::normalize(normal);

    glm::vec3 lightDir = glm::normalize(glm::vec3(viewDir.x, viewDir.y, viewDir.z) - v1);
    glm::vec3 viewDir = glm::normalize(glm::vec3(0.0, 0.0, 0.0) - v1);
    glm::vec3 reflectDir = glm::reflect(-lightDir, normal);

    float kd = 0.5f; // коэффициент диффузного отражения
    float ks = 0.5f; // коэффициент зеркального отражения
    float shininess = 100.0f; // степень блеска материала

    // Фоновая составляющая (ambient)
    glm::vec3 ambientColor(0.1f, 0.1f, 0.1f); // Цвет фонового освещения

    // Рассчитываем диффузное освещение
    float diff = kd * std::max(glm::dot(normal, lightDir), 0.0f);

    // Рассчитываем зеркальное освещение
    float specular = 0.0f;
    if (diff > 0) {
        specular = ks * pow(std::max(glm::dot(reflectDir, viewDir), 0.0f), shininess);
    }

    glm::vec3 color(1.0f, 1.0f, 1.0f); // Цвет пикселя

    // Вычисляем общее освещение (сумма диффузного, зеркального и фонового освещения)
    glm::vec3 finalColor = color * (ambientColor + diff + specular);

    // Применяем освещение к цвету пикселя (i, j)
    glColor3f(finalColor.r, finalColor.g, finalColor.b);
}

void Torrance(int i, int j) {
    glm::vec3 v1(i, j, matrix[i][j]);
    glm::vec3 v2(i - 1, j, matrix[i - 1][j]);
    glm::vec3 v3(i - 1, j + 1, matrix[i - 1][j + 1]);
    glm::vec3 normal = glm::cross(v2 - v1, v3 - v1);
    normal = glm::normalize(normal); // Нормаль поверхности 

    glm::vec3 lightDir = glm::normalize(glm::vec3(viewDir.x, viewDir.y, viewDir.z) - v1);
    glm::vec3 viewDir = glm::normalize(glm::vec3(0.0, 0.0, 0.0) - v1);

    // Расчет направления полу-угла обзора
    glm::vec3 halfwayDir = glm::normalize(viewDir + lightDir);

    // Модель распределения микро-граней (Бекман)
    float NdotH = glm::dot(normal, halfwayDir);
    float roughness = 0.3f;
    float roughnessSq = roughness * roughness;
    float D = exp((NdotH * NdotH - 1.0f) / (NdotH * NdotH * roughnessSq)) / (glm::pi<float>() * roughnessSq * NdotH * NdotH * NdotH * NdotH);

    // Рассчитываем интенсивность отраженного света согласно модели Торранса-Спарроу 
    float NdotL = glm::dot(normal, lightDir);
    float NdotV = glm::dot(normal, viewDir);
    float specular = D * NdotL * NdotV / glm::max(1.0f, glm::max(NdotL, NdotV));

    // Применяем интенсивность к цвету или освещению для пикселя (i, j)
    glm::vec3 color(1.0f, 1.0f, 1.0f); // Цвет пикселя
    glColor3f(color.r * specular, color.g * specular, color.b * specular);
}

void Object() {
    double avg_x = 0;
    double avg_y = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            avg_x += i;
            avg_y += j;
        }
    }

    avg_x /= height * width;
    avg_y /= height * width;

    glTranslatef(-avg_x * scale, -avg_y * scale, 0);
    glScalef(scale, scale, scale);

    for (int i = 1; i < height; i++) {
        for (int j = 0; j < width - 1; j++) {
            if ((matrix[i][j] > 0.0) && (matrix[i - 1][j] > 0.0) && (matrix[i - 1][j + 1] > 0.0) && (matrix[i][j + 1] > 0.0)) 
            {
                switch (model)
                {
                case 1:
                    Lambert(i, j);
                    break;

                case 2:
                    Fong(i, j);
                    break;

                case 3:
                    Torrance(i, j);
                    break;

                default:
                    break;
                }

                glBegin(GL_TRIANGLE_FAN);
                glVertex3d(i, j, matrix[i][j]);
                glVertex3d(i - 1, j, matrix[i - 1][j]);
                glVertex3d(i - 1, j + 1, matrix[i - 1][j + 1]);
                glEnd();

                glBegin(GL_TRIANGLE_FAN);
                glVertex3d(i, j, matrix[i][j]);
                glVertex3d(i, j + 1, matrix[i][j + 1]);
                glVertex3d(i - 1, j + 1, matrix[i - 1][j + 1]);
                glEnd();
            }
        }
    }
    glFlush();
}

void DrawLightSource() {
    glPushMatrix();
    glTranslatef(viewDir.x, viewDir.y, viewDir.z);
    glColor3f(1.0f, 1.0f, 0.0f); // Цвет источника света (сферы)
    glutSolidSphere(10, 20, 20); // Рисуем сферу в качестве источника света
    glPopMatrix();
}

void Draw() {

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 700 / 500, 0.000001, 100000);
    gluLookAt(eyePos.x, eyePos.y, eyePos.z, 0, 0, 0, 0, 1, 0); // Установка нового положения точки наблюдения
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glRotated(rotateX, 1.0, 0.0, 0.0);
    glRotated(rotateY, 0.0, 1.0, 0.0);
    glScaled(scaleFactor, scaleFactor, scaleFactor);   
    Object();

    DrawLightSource();

    glutSwapBuffers();
}

void keyPressed(unsigned char key, int x, int y) {
        switch (key) {
        
        // Вращение и масштабирование
        case 'w':
            rotateX += 5.0;
            break;
        case 's':
            rotateX -= 5.0;
            break;
        case 'a':
            rotateY -= 5.0;
            break;
        case 'd':
            rotateY += 5.0;
            break;
        case '=':
            scaleFactor += 0.1;
            break;
        case '-':
            scaleFactor -= 0.1;
            break;

        // Управление положением источника света
        case 'x':
            viewDir.x -= 10.0;
            break;
        case 'X':
            viewDir.x += 10.0;
            break;
        case 'y':
            viewDir.y -= 10.0;
            break;
        case 'Y':
            viewDir.y += 10.0;
            break;
        case 'z':
            viewDir.z -= 10.0;
            break;
        case 'Z':
            viewDir.z += 10.0;
            break;

        // Управление положением точки наблюдения относительно объекта
        case 'i':
            eyePos.y += 10.0;
            break;
        case 'k':
            eyePos.y -= 10.0;
            break;
        case 'j':
            eyePos.x -= 10.0;
            break;
        case 'l':
            eyePos.x += 10.0;
            break;
        case 'u':
            eyePos.z += 10.0;
            break;
        case 'n':
            eyePos.z -= 10.0;
            break;

        case 'c':
            WriteToFile("model.bmp", 600, 600);
            break;
        default:
            break;
        }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");
    std::cout << "Выберите модель отражения\n";
    std::cout << "0 - Без модели отражения\n";
    std::cout << "1 - Ламберта\n";
    std::cout << "2 - Фонга\n";
    std::cout << "3 - Торранса-Спарроу\n\n";

    std::cin >> model;

    ReadFromFile();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowPosition(500, 150);
    glutInitWindowSize(600, 600);
    glutCreateWindow("models");

    glutKeyboardFunc(keyPressed);
    glutDisplayFunc(Draw);
    glutMainLoop();
    return 0;
}
