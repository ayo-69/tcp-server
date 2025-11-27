CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall
LDFLAGS = -lpthread

TARGET = server

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)
