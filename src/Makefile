CC = gcc
FLAGS = -o -pthread -Wall -lrt
OBJS = system_manager.c functions.c functions.h
TARGET = system_manager
OBJS1 = user_console.c functions.c functions.h
TARGET2 = user_console
OBJS2 = sensor.c functions.c functions.h
TARGET3 = sensor

all: $(TARGET) $(TARGET2) $(TARGET3)

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) $(TARGET)


copy:
	cp $(TARGET) /home/user/Desktop/projetoSO/$(TARGET)
	cp $(TARGET2) /home/user/Desktop/projetoSO/$(TARGET2)
	cp $(TARGET3) /home/user/Desktop/projetoSO/$(TARGET3)

clean:
		$(RM) $(TARGET)
		$(RM) $(TARGET2)
		$(RM) $(TARGET3)
