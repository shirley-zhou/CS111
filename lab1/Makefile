CC = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused -Werror
LAB = 1
USER = junzhou
DISTDIR = lab1-$(USER)

all: simpsh

TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

LAB1_SOURCES = \
  lab1.c

DIST_SOURCES = \
  $(LAB1_SOURCES) Makefile \
  $(TESTS) README input.txt output.txt error.txt input2.txt output2.txt error2.txt exec.sh exec2.sh exec3.sh exec4.sh exec5.sh exec6.sh exec7.sh exec8.sh a b c poem.txt comparison.pdf

dist: $(DISTDIR).tar.gz

simpsh: lab1.c
	$(CC) $(CFLAGS) -o $@ lab1.c

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	rm -fr $(DISTDIR)
	tar -czf $@.tmp --transform='s,^,$(DISTDIR)/,' $(DIST_SOURCES)
	mv $@.tmp $@

check: $(TEST_BASES)

$(TEST_BASES): simpsh
	./$@.sh

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp simpsh $(DISTDIR)

.PHONY: all dist check $(TEST_BASES) clean
