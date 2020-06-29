# Manual de utilização do IRC
 
## Desenvolvedores
Luan Ícaro Pinto Arcanjo 10799230  
Rodrigo Cesar Arboleda 10416722
 
## Compilar código
Para compilar o programa deve-se utilizar o comando “make”, este irá executar a receita “all”.
```
$ make
```
Após executar “make”, será gerado os arquivos intermediários .o. Estes arquivos não são excluídos após o programa ser compilado. Para tal, basta executar o comando “make clean”.
```
$ make clean
```
Com isso temos o código do cliente compilado em um arquivo chamado “irc_client” e o do servidor em um arquivo chamado "irc_server".
 
 
## Rodar o programa
### Cliente
Para rodar o cliente do irc basta executar o comando “make run”.
```
$ make run
```
Caso não deseje rodar o comando “make run” basta compilar o código com o comando “make irc_client” e em seguida rodar o arquivo “irc_client” com o comando:
```
$ ./irc_client
```
### Servidor
Para rodar o servidor do irc basta executar o comando “make run_server”.
```
$ make run_server
```
Caso não deseje rodar o comando “make run_server” basta compilar o código com o comando “make irc_server” e em seguida rodar o arquivo “irc_server” com o comando:
```
$ ./irc_server
```
 
## Utilizando o programa - Cliente
Ao iniciar uma mensagem será mostrada e com isso já pode começar a utilizar o irc.
### Conectar a um computador
O primeiro passo é se conectar a um servidor, para isto basta digitar "/connect".
```
/connect
```
Após isso será solicitado um IP, basta digitar o ip do servidor no formato xxx.xxx.xxx.xxx (exemplo:127.0.0.1).
```
Digite o IP do servidor:
127.0.0.1
```
### Nickname
Ao abrir o programa pela primeira vez será exigido que você crie um nickname, este será salvo no arquivo usr.cfg. Após isso, sempre que for abrir o programa irá carregar o último nome utilizado. O nickname pode ser alterado a qualquer momento pelo comando "/nickname name".
```
/nickname name
```
#### Nome já em uso
Caso no servidor já tenha alguém como o mesmo nome que você, o seu nome será atribuído para um nome padrão. Se você tentar atualizar o nome e não tiver em um servidor ou já tiver alguém com o mesmo nome, será mudado seu nome nas configurações locais da aplicação.  
#### Nome no canal
Os nomes são individuais por canal, ou seja, podem ter duas pessoas com o mesmo nick no server mas não no mesmo canal.  
OBS: Pessoas sem canal contam como se estivessem no mesmo canal.

### Testando ping
Este comando irá enviar ao servidor uma mensagem e aguardar a resposta dele com PONG, após receber a resposta do servidor, a mensagem recebida ser mostrada ao usuário.
```
/ping
```
### Entrando em um canal
Você pode entrar em um canal com o comando "/join canal", caso ele não exista irá também criar um canal.
```
/join canal
```
### Saindo de um canal
Você pode sair de um canal com o comando "/unjoin", caso não esteja em nenhum canal, será informado que não está em nenhum canal e nada ocorrerá.
```
/unjoin
```
### Kick
Se você for o administrador de um canal, você pode expulsar pessoas do canal com o comando "/kick nome", essa pessoa é expulsa do canal e não banida, podendo assim voltar ao canal.
```
/kick nome
```
### Mute
Se você for o administrador de um canal, você pode mutar pessoas do canal com o comando "/mute nome", essa pessoa não irá poder falar e nem trocar de nome, caso ela sai e entre no canal novamente ela poderá falar.
```
/mute nome
```
### Unmute
Se você for o administrador de um canal, você pode desmutar pessoas do canal com o comando "/unmute nome".
```
/unmute nome
```
### Ver ip do usuário
Se você for o administrador de um canal, você pode ver o ip das pessoas do canal com o comando "/whois nome".
```
/whois nome
```
### Encerrando o programa
Para encerrar o programa basta digitar "/quit" e a conexão será fechada e o programa encerrado. 
```
/quit
```
Caso o cliente tenha 3 erros consecutivos de conexão com servidor a conexão será encerrada mas o programa não irá fechar, podendo digitar "/quit" para encerrar o programa.

### Enviando mensagens
Para enviar mensagem basta digitar a mensagem sem "/". Caso não esteja conectado a nenhum servidor será mostrado uma erro.  

Todas as mensagens enviadas ao servidor são redirecionadas a todos os clientes on-line no servidor.

### Recebendo mensagens
Cada cliente é identificado com um número e o qual é mostrado antes da mensagem.  

Suas mensagens são mostradas para você sem identificação na frente mas os clientes que estão conectados, quando recebem suas mensagens, possuem i.

## Utilizando o programa - Servidor
Ao iniciar o servidor, ele irá gerenciar de forma automática as conexões, só é preciso abrir a porta 1515 na rede para que o servidor possa estabelecer conexões com outros computadores.

### Verificando número de clientes no servidor
Para verificar o número de clientes ativo no servidor deve digitar o comando "/numero_clientes" no servidor.
```
/numero_clientes
```
### Encerrando o Servidor
Para encerrar o servidor deve ser executado o comando "/shutdown".
```
/shutdown
```
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
