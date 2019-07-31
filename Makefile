MODULE_big = pgcodec7
OBJS = pgcodec7.o
EXTENSION = pgcodec7
DATA = pgcodec7--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
