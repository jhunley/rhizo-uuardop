<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta charset='UTF-8'/>
    <title>Serviços de Comunicação Digital</title>
    <link rel='stylesheet' href='styles.css'/>
  </head>
  <body>
    <h2>Estação: <?php include('get_name.php') ?></h2>

    <h2>Estado do Sistema: Parado / Pronto para Receber Chamada / Em
    Chamada (Put here if uuardopd is running or not)</h2>

    <h2>Recebimento de arquivos</h2>
    <p>
    Conteúdo recebido: <br />
    <?php include('list.php') ?>
    </p>
    <br />
    <h2>Envio de arquivo</h2>
    <p>
    </p>
    <form action="upload.php" method="post" enctype="multipart/form-data">
      Estação de destino: 
      <select id="prefix" name="prefix">
        <?php include('get_systems.php') ?>
      </select>
      <br />
      Escolha o arquivo:
      <input type="file" name="fileToUpload" id="fileToUpload" />
      <input type="submit" value="Colocar arquivo na fila" name="submit" />
      <input type="hidden" id="myname" name="myname" value="<?php include('get_name.php')?>"/>

      <br />

    </form>
    </p>
<br/>
<br/>

<br/>
Ver fila de trabalho (uustat.php)

Status (uulog)


  </body>
</html>
