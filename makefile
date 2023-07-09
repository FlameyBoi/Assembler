CC	= gcc
CFLAGS	= -Wall -ansi -pedantic
OBJFILES= Allocator.o ParseTool.o parsingTable.o PreAssembler.o CommandHandler.o AssemblerSecondPass.o AssemblerFirstPass.o main.o
TARGET	= assembler

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
