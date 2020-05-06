# Manual de utilização do IRC

## Desenvolvedores
Luan Ícaro Pinto Arcanjo 10799230  
Rodrigo Cesar Arboleda 10416722

## Compilar código
Para compilar o programa deve-se utilizar o comando “make”, este irá execultar a receita “all”.
```
$ make
```
Após execultar “make”, será gerado os arquivos intermediários .o. Estes arquivos não são excluídos após o programa ser compilado. Para tal, basta execultar o comando “make clean”.
```
$ make clean
```
Com isso temos o código compilado em um arquivo chamado “irc”.


## Rodar o programa
Para rodar o programa basta execultar o comando “make run”.
```
$ make run
```
Caso não deseje rodar o comando “make run” basta compilar o código com o comando “make” e em seguida rodar o arquivo “irc” com o comando:
```
$ ./irc
```

## Utilizando o programa
Ao inicia o programa existem 2 opções, 1 para se conectar a um servidor e 2 para abrir um servidor.
### 1:
Está opção te levará a para digitar um IP. Digite um IP de um outro computador que está aguardando sua conexão para estabelecer uma.
### 2:
Está opção te colocará em modo de espera, esperando um outro computador para se conectar. Para isso, passe o IP de seu computador para que o outro usuário possa se conectar.

## Encerrando o programa
Para encerrar o programa basta pressionar Ctrl+C que ele irá fechar os sockets abertos e encerrar o programa.  
  
Caso o cliente se desconecte do servidor, uma mensagem dizendo para digitar uma mensagem irá aparecer, então basta digitar uma mensagem que o programa irá encerrar.


## Erros comuns
### ERRO AO ASSOCIAR ENDEREÇO
Tal erro pode ser causado caso tenha se encerrado o programa de forma indevida. Dessa forma a porta continua ocupada. Para solucionar tal problema, reinicie o terminal. Caso o erro persistir, reinicie sua sessão no GNOME (fazer logoff).
### ERRO AO SE CONECTAR AO SERVIDOR
Este erro é causado em maioria quando o endereço de IP está errado ou o servidor não abriu corretamente a porta.
Caso o servidor não tenha aberto a porta, a porta 1515 deve ser aberta na rede para que a conexão possa ser feita.
Esse erro pode ser causado caso o cliente encerre a conexão com o servidor. Neste caso é preciso estabelecer uma nova conexão.

Caso ocorra algum outro erro, entre em contato com os desenvolvedores.

## Versão dos programas utilizados
### compilador
gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
### Sistema operacional
Ubuntu 20.04 LTS
### Kernel
5.4.0-28-generic
