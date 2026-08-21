NAME = search

CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -fopenmp -Wall -Wextra -MMD -MP
CXXFLAGS_DEBUG = -std=c++20 -g3 -fopenmp -Wall -Wextra -MMD -MP

SRCS = search.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: re

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re debug
