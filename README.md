# Визуализация шума Перлина на OpenGL

Случайный шум генерируется один раз в начале программы, а потом меняется по некоторому закону

## Запуск (для CLion)

1. Должны быть установлены библиотеки `glew-2.1.0` и `SDL2-2.0.16` (возможно, другие версии тоже подойдут)
2. В CMake options следует прописать пути: `-DGLEW_ROOT="your_path\glew-2.1.0" -DSDL2_ROOT="your_path\SDL2-2.0.16"`
3. В папку с исполняемый файлом (например, `cmake-build-debug`) нужно положить `SDL2.dll` и `glew32.dll`

## Управление

- `WASDRF` для движения камеры
- стрелочки для поворотов графика
- цифры `9` и `0` для удаления/добавления изолиний
- клавиши `-` и `+` для уменьшения/увеличения размеров сетки

## Пример

<img src="https://i.ibb.co/dLq8XDJ/image.png">