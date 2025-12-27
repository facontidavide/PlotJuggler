FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install basic development tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    pkg-config \
    python3 \
    python3-pip \
    ninja-build \
    gdb \
    bash-completion \
    ccache \
    clang-format \
    clang-tidy \
    locales \
    && locale-gen en_US.UTF-8 \
    && pip3 install pre-commit \
    && rm -rf /var/lib/apt/lists/*

ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# Install PlotJuggler dependencies (from COMPILE.md)
RUN apt-get update && apt-get install -y \
    qtbase5-dev \
    libqt5svg5-dev \
    libqt5websockets5-dev \
    libqt5opengl5-dev \
    libqt5x11extras5-dev \
    libprotoc-dev \
    libzmq3-dev \
    liblz4-dev \
    libzstd-dev \
    qttools5-dev-tools \
    && rm -rf /var/lib/apt/lists/*

# Install X11 libraries for GUI support
RUN apt-get update && apt-get install -y \
    libx11-dev \
    libxkbcommon-x11-0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xfixes0 \
    x11-apps \
    && rm -rf /var/lib/apt/lists/*

# Install Conan
RUN pip3 install conan

# Create a non-root user
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m -s /bin/bash $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

USER $USERNAME

# Setup bash with completion and better defaults
RUN cat >> ~/.bashrc <<'EOF'

# Enable bash completion
if ! shopt -oq posix; then
  if [ -f /usr/share/bash-completion/bash_completion ]; then
    . /usr/share/bash-completion/bash_completion
  elif [ -f /etc/bash_completion ]; then
    . /etc/bash_completion
  fi
fi

# Git prompt function
parse_git_branch() {
    git branch 2>/dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/ (\1)/'
}

# Better prompt with git branch
PS1='\[\033[01;32m\]\u@plotjuggler\[\033[00m\]:\[\033[01;34m\]\w\[\033[33m\]$(parse_git_branch)\[\033[00m\]\$ '

# History settings
HISTSIZE=10000
HISTFILESIZE=20000
HISTCONTROL=ignoreboth:erasedups
shopt -s histappend

# Better directory navigation
shopt -s autocd 2>/dev/null
shopt -s cdspell 2>/dev/null
shopt -s dirspell 2>/dev/null

# ccache setup
export PATH="/usr/lib/ccache:$PATH"

# Useful aliases
alias ll='ls -la --color=auto'

# Colored GCC output
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'
EOF

# Initialize Conan default profile
RUN conan profile detect

# Copy conanfile and build script
WORKDIR /workspace
COPY --chown=$USERNAME:$USERNAME conanfile.txt .


# Run conan install (Release for libraries)
RUN conan install . --output-folder=build --build=missing -s build_type=Release

WORKDIR /workspace
