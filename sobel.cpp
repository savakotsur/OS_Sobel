#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

// Структура для хранения изображения
struct Image {
    vector<vector<vector<int>>> data; // 3D вектор для хранения цветов каждого пикселя (RGB)
    int width;
    int height;
};

// Функция для загрузки изображения из файла формата PPM
Image loadPPM(const string& filename) {
    ifstream file(filename, ios::binary); // Открываем файл в бинарном режиме
    if (!file.is_open()) {
        cerr << "Error: Unable to open file '" << filename << "'" << endl;
        return {};
    }
    
    char format[3];
    file >> format; // Считываем формат (P6)
    if (format[0] != 'P' || format[1] != '6') {
        cerr << "Error: Invalid image format '" << format << "'. Expected 'P6'." << endl;
        return {};
    }
    
    Image image;
    file.ignore(256, '\n'); // Пропускаем строку с комментариями, если она есть
    
    // Читаем ширину, высоту и максимальное значение цвета
    file >> image.width >> image.height;
    int maxColorValue;
    file >> maxColorValue;
    file.ignore(); // Пропускаем конечную строку
    
    // Читаем данные пикселей
    image.data.resize(image.height, vector<vector<int>>(image.width, vector<int>(3)));
    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            char r, g, b;
            file.read(&r, 1); // Читаем красный канал
            file.read(&g, 1); // Читаем зеленый канал
            file.read(&b, 1); // Читаем синий канал
            image.data[i][j][0] = static_cast<int>(r);
            image.data[i][j][1] = static_cast<int>(g);
            image.data[i][j][2] = static_cast<int>(b);
        }
    }
    
    return image;
}

// Функция для применения фильтра Собеля к изображению
void applySobelFilter(const Image& image, Image& result, int startRow, int endRow) {
    // Ядро фильтра Собеля по оси X
    int kernelX[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    
    // Ядро фильтра Собеля по оси Y
    int kernelY[3][3] = {
        {1, 2, 1},
        {0, 0, 0},
        {-1, -2, -1}
    };
    
    // Применяем фильтр Собеля к каждому пикселю изображения
    for (int i = startRow; i < endRow; ++i) {
        for (int j = 1; j < image.width - 1; ++j) {
            int gx = 0, gy = 0;
            
            // Применяем ядра фильтра Собеля к окрестности каждого пикселя
            for (int k = -1; k <= 1; ++k) {
                for (int l = -1; l <= 1; ++l) {
                    gx += image.data[i + k][j + l][0] * kernelX[k + 1][l + 1];
                    gy += image.data[i + k][j + l][0] * kernelY[k + 1][l + 1];
                }
            }
            
            // Вычисляем магнитуду градиента
            int magnitude = sqrt(gx * gx + gy * gy);
            
            // Ограничиваем значение магнитуды до диапазона 0-255
            magnitude = min(255, max(0, magnitude));
            
            // Присваиваем магнитуду всем каналам пикселя
            result.data[i][j][0] = magnitude;
            result.data[i][j][1] = magnitude;
            result.data[i][j][2] = magnitude;
        }
    }
}

// Функция для сохранения изображения в файл формата PPM
void savePPM(const Image& image, const string& filename) {
    ofstream file(filename, ios::binary); // Открываем файл в бинарном режиме
    if (!file.is_open()) {
        cerr << "Error: Unable to create file '" << filename << "'" << endl;
        return;
    }
    
    // Записываем формат (P6), ширину, высоту и максимальное значение цвета
    file << "P6\n" << image.width << " " << image.height << "\n255\n";
    
    // Записываем данные пикселей
    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            file.put(static_cast<char>(image.data[i][j][0])); // Красный канал
            file.put(static_cast<char>(image.data[i][j][1])); // Зеленый канал
            file.put(static_cast<char>(image.data[i][j][2])); // Синий канал
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input_image.ppm> <num_threads>" << endl;
        return 1;
    }
    
    string filename = argv[1];
    int numThreads = stoi(argv[2]);
    
    // Загружаем изображение из файла
    Image image = loadPPM(filename);
    if (image.data.empty()) {
        return 1;
    }
    
    cout << "Loaded image with width: " << image.width << ", height: " << image.height << endl;
    
    // Создаем изображение для результата
    Image result = image;
    
    auto start = high_resolution_clock::now(); // Запускаем таймер
    
    if (numThreads == 1) {
        // Однопоточное выполнение
        applySobelFilter(image, result, 1, image.height - 1);
    } else {
        // Многопоточное выполнение
        vector<thread> threads;
        int rowsPerThread = image.height / numThreads;
        
        // Создаем и запускаем потоки для обработки изображения
        for (int i = 0; i < numThreads; ++i) {
            int startRow = i * rowsPerThread + 1;
            int endRow = (i == numThreads - 1) ? image.height - 1 : (i + 1) * rowsPerThread;
            threads.emplace_back(applySobelFilter, cref(image), ref(result), startRow, endRow);
        }
        
        // Дожидаемся завершения всех потоков
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    auto stop = high_resolution_clock::now(); // Останавливаем таймер
    auto duration = duration_cast<microseconds>(stop - start); // Вычисляем время выполнения
    
    cout << "Time taken: " << duration.count() << " microseconds" << endl;
    
    // Сохраняем изображение с примененным фильтром Собеля
    string outputFilename = filename.substr(0, filename.find_last_of('.')) + "_sobel.ppm";
    savePPM(result, outputFilename);
    
    return 0;
}

