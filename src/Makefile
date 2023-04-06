CC = gcc
FLAGS = -pthread -Wall
OBJS = system_manager.c functions.c functions.h
TARGET = home_iot
OBJS1 = user_console.c functions.c functions.h
TARGET1 = user_console
OBJS2 = sensor.c functions.c functions.h
TARGET2 = sensor

all: $(TARGET) $(TARGET1) $(TARGET2)

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(TARGET)
	cp $(TARGET) /home/user/Desktop/projetoSO/$(TARGET)

$(TARGET1): $(OBJS1)
	$(CC) $(FLAGS) $(OBJS1) -o $(TARGET1)
	cp $(TARGET1) /home/user/Desktop/projetoSO/$(TARGET1)

$(TARGET2): $(OBJS2)
	$(CC) $(FLAGS) $(OBJS2) -o $(TARGET2)
	cp $(TARGET2) /home/user/Desktop/projetoSO/$(TARGET2)

config:
	cp config.txt /home/user/Desktop/projetoSO/config.txt

clean:
		$(RM) $(TARGET)
		$(RM) $(TARGET1)
		$(RM) $(TARGET2)
