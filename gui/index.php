<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta charset='UTF-8'/>
    <title>Serviços de Comunicação Digital</title>
    <link rel='stylesheet' href='style.css'/>
  </head>
  <body>
    <center>
      <div class="header">
      <h2>Estação: <?php include('get_name.php') ?></h2>
      <h5>Estado do Sistema: <?php include 'running.php'; ?></h5>
      <hr />
      </div>

      <p>
	<h1><a href="load.php">Carregar Arquivo para Transmissão</a></h1>
      </p>
      
      <div class="body">
      <p>
	<h1><a href="tx.php">Transmitir</a></h1>
      </p>
      </div>

      <p>
	<h1><a href="list_job.php">Listar Arquivos Carregados para Transmissão</a></h1>
      </p>

      <div class="body">
      <p>
	<h1><a href="arquivos/">Recebimento de arquivos</a></h1>
      </p>
      </div>

      <p>
	<h1><a href="kill_job.php">Cancelar transmissão</a></h1>
      </p>

      <div class="body">
      <p>
	<h1><a href="list_job.php">Fila de transmissão</a></h1>
      </p>
      </div>

      <p>
	<h1><a href="view_log.php">Ver log</a></h1>
      </p>
    </center>
    <br/>
    <br/>
  </body>
</html>
