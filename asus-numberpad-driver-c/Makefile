CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c11 -D_GNU_SOURCE $(shell pkg-config --cflags libevdev 2>/dev/null || echo "")
LDFLAGS = $(shell pkg-config --libs libevdev 2>/dev/null || echo "-levdev") -lX11 -lwayland-client -lxkbcommon -lsystemd -lpthread -lm

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/asus-numberpad-driver

# Default target
all: $(TARGET)

# Create directories
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Link object files
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Install (requires root)
install: $(TARGET)
	@echo "Installation not yet implemented"
	@echo "TODO: Copy binary to /usr/bin/"
	@echo "TODO: Install systemd service file"
	@echo "TODO: Install udev rules"

# Uninstall
uninstall:
	@echo "Uninstallation not yet implemented"

.PHONY: all clean install uninstall

