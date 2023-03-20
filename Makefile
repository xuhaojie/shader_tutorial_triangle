
#获取.cpp文件
SrcFiles=$(wildcard *.cpp)
SrcFiles+=$(wildcard ./common/*.cpp)
#使用替换函数获取.o文件
ObjFiles=$(patsubst %.cpp,%.o,$(SrcFiles))
#头文件路径 
INCLUDES = -I./ -I./thirdparty  
#库文件名字  
LIBS = -ldl -lstdc++ -lGL -lGLEW -lglfw


#目标文件依赖于.o文件
shader:$(ObjFiles)
	g++ -o $@ $(INCLUDES) $(SrcFiles) $(LIBS)

#.o文件依赖于.cpp文件，通配使用，一条就够
%.o:%.cpp
	g++ -c -I ../include $<

.PHONY:clean all

clean:
	rm -f *.o
	rm shader