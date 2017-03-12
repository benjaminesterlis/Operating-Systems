CC = gcc
OBJS = test.o kci_ctrl.o
TEST = write
CTRL = ctrl

all: $(OBJS)
		
test.o:  test.c
	$(CC) $< -o $(TEST)

kci_ctrl.o: kci_ctrl.c
	$(CC) $< -o $(CTRL)

clean:
	rm -f $(TEST) $(CTRL)
