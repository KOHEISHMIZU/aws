ZYNQ = n

CC = g++

INST_DIR = ../bin


ifeq ($(ZYNQ), y)
	CC	= arm-xilinx-linux-gnueabi-g++
endif

OBJ = aws_cmd.o ../command.o filter.o channel.o fcmd.o fset.o fget.o cyc.o pause.o go.o quit.o stop.o step.o trat.o clear.o finf.o chinf.o fpar.o frm.o chrm.o awscd.o awstime.o
TGT = filter channel fcmd fset fget cyc pause go quit stop step trat clear finf chinf fpar frm chrm awscd awstime
OBJv2 = c_aws_cmd.o fls.o chls.o fpls.o
TGTv2 = fls chls fpls
OBJv3 = c_aws_cmd.o awsevt.o 
TGTv3 = awsevt

all: $(TGT) $(TGTv2) $(TGTv3)
$(TGT): $(OBJ)
	$(CC) aws_cmd.o ../command.o $@.o -o $@
$(TGTv2): $(OBJv2)
	$(CC) c_aws_cmd.o ../command.o $@.o -o $@
$(TGTv3): $(OBJv3)
	$(CC) c_aws_cmd.o ../command.o $@.o -o $@ -lpthread
%.o: %.cpp
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJ) $(TGT) $(OBJv2) $(TGTv2) $(OBJv3) $(TGTv3)
install:
	for cmd in $(TGT); do \
	 cp $$cmd $(INST_DIR)/$$cmd; \
	done
	for cmd in $(TGTv2); do\
	 cp $$cmd $(INST_DIR)/$$cmd; \
	done
	cp $(TGTv3) $(INST_DIR)/$(TGTv3)

