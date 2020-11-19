FROM danger89/cmake:3.1

RUN apt-get update \
 && apt-get install -y qt5-default \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
