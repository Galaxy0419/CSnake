unix-debug:
	cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B ./Build/
	cmake --build ./Build/

unix-release:
	cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B ./Build/
	cmake --build ./Build/

mingw-release:
	cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=./mingw.cmake -DCMAKE_BUILD_TYPE=Release -B ./Build/
	cmake --build ./Build/

ms-debug:
	cmake -B ./Build/
	cmake --build ./Build/ --config Debug

ms-release:
	cmake -B ./Build/
	cmake --build ./Build/ --config Release

clean:
	rm -rf Build/
