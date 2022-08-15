FROM riot/riotbuild

USER root
WORKDIR /root

# TODO: install openssh-server
# ssh-keygen -A
# mkdir -p /run/sshd
RUN apt-get update && apt-get install -y --no-install-recommends \
    openvswitch-switch \
    curl \
    iproute2 \
    iputils-ping \
    mininet \
    net-tools \
    tcpdump \
    vim \
    x11-xserver-utils \
    openvswitch-testcontroller \
    openssh-server \
    bridge-utils \
    xterm \
 && rm -rf /var/lib/apt/lists/* \
 # && service openvswitch-switch start \
 && ssh-keygen -A \
 && mkdir -p /run/sshd
COPY  docker-entry-point /docker-entry-point
RUN chmod +x /docker-entry-point

EXPOSE 6633 6653 6640

ENTRYPOINT [ "/docker-entry-point" ]

