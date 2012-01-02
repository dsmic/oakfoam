var board_size;
var komi;
var moves;
var last_move;
var next_color;

function xy2pos(x,y)
{
  if (x>7) x++; // there is no I column
  return String.fromCharCode('A'.charCodeAt()+x)+(y+1);
}

function doGtpCmdRefresh(cmd,args)
{
  $('#status').append('Thinking...<br/>\n');
  $.get(cmd+'.gtpcmd?'+args,refreshBoard);
}

function drawBoard(data)
{
  var board='';
  for (x=0;x<board_size;x++)
  {
    for (y=0;y<board_size;y++)
    {
      pos=xy2pos(x,y)
      c=data[pos];
      if (c=='B')
        board+='<img alt="'+pos+'" src="images/board/black.png"/>';
      else if (c=='W')
        board+='<img alt="'+pos+'" src="images/board/white.png"/>';
      else if (x==0 && y==0)
        board+='<img alt="'+pos+'" src="images/board/top-left.png"/>';
      else if (x==0 && y==(board_size-1))
        board+='<img alt="'+pos+'" src="images/board/top-right.png"/>';
      else if (x==(board_size-1) && y==0)
        board+='<img alt="'+pos+'" src="images/board/bottom-left.png"/>';
      else if (x==(board_size-1) && y==(board_size-1))
        board+='<img alt="'+pos+'" src="images/board/bottom-right.png"/>';
      else if (x==0)
        board+='<img alt="'+pos+'" src="images/board/top.png"/>';
      else if (y==0)
        board+='<img alt="'+pos+'" src="images/board/left.png"/>';
      else if (x==(board_size-1))
        board+='<img alt="'+pos+'" src="images/board/bottom.png"/>';
      else if (y==(board_size-1))
        board+='<img alt="'+pos+'" src="images/board/right.png"/>';
      else
        board+='<img alt="'+pos+'" src="images/board/center.png"/>';
    }
    board+="<br/>\n";
  }
  $('#board').html(board);
  $('#board img').click(function()
  {
    pos=$(this).attr('alt');
    doGtpCmdRefresh('play',next_color+'&'+pos);
  });
}

function updateStatus()
{
  var stat='';

  stat+='Komi: '+komi+'<br/>\n';
  stat+='Moves: '+moves+'<br/>\n';
  stat+='Next to Move: '+next_color+'<br/>\n';

  $('#status').html(stat);
}

function refreshBoard()
{
  $.getJSON('board_info.jsoncmd',function(data)
  {
    board_size=data['size'];
    komi=data['komi'];
    moves=data['moves'];
    last_move=data['last_move'];
    next_color=data['next_color'];
    updateStatus();
  });
  $.getJSON('board_pos.jsoncmd',function(data){drawBoard(data);});
}

$(document).ready(function()
{
  $.getJSON('engine_info.jsoncmd',function(data)
  {
    $('#engine').html(data['name']+' '+data['version']);
  });
  
  refreshBoard();

  /*$.getJSON('board_info.jsoncmd',function(data)
  {
    var items = [];

    $.each(data, function(key, val) {
      items.push('<li>' + key + ':' + val + '</li>');
    });

    $('<ul/>', {
      html: items.join('')
    }).appendTo('body');
  });*/

  $('#dialog-new').dialog(
  {
    autoOpen: false,
    //height: 300,
    width: 250,
    modal: true,
    resizable: false,
    buttons: {
      'OK': function()
      {
        doGtpCmdRefresh('clear_board','');
        doGtpCmdRefresh('boardsize',$('#dialog-new-size').val());
        doGtpCmdRefresh('komi',$('#dialog-new-komi').val());

        $(this).dialog('close');
      },
      'Cancel': function()
      {
        $(this).dialog('close');
      }
    }
  });

  $('button').button();
  $('#new').click(function()
  {
    $('#dialog-new-size').val(board_size);
    $('#dialog-new-komi').val(komi);
    $('#dialog-new').dialog('open');
  });
  $('#pass').click(function(){doGtpCmdRefresh('play',next_color+'&pass');});
  $('#genmove').click(function(){doGtpCmdRefresh('genmove',next_color);});

});


