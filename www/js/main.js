var board_size;
var komi;
var moves;
var last_move;
var next_color;
var engine_color='none';
var passes;
var game_over=false;

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

function isHoshi(x,y)
{
  size=board_size;
  dx=x;
  if (dx>(size/2))
    dx=size-dx-1;
  dy=y;
  if (dy>(size/2))
    dy=size-dy-1;
  if (dx>dy)
  {
    t=dx;
    dx=dy;
    dy=t;
  }

  if (size==9)
  {
    if (dx==2 && dy==2)
      return true;
    else if (dx==4 && dy==4)
      return true;
    else
      return false;
  }
  else if (size==13)
  {
    if (dx==3 && dy==3)
      return true;
    else if (dx==6 && dy==6)
      return true;
    else
      return false;
  }
  else if (size==19)
  {
    if (dx==3 && dy==3)
      return true;
    else if (dx==9 && dy==9)
      return true;
    else if (dx==3 && dy==9)
      return true;
    else
      return false;
  }
  else
    return false;
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
      {
        if ((c+":"+pos)==last_move)
          board+='<img alt="'+pos+'" src="images/board/black-red.png"/>';
        else
          board+='<img alt="'+pos+'" src="images/board/black.png"/>';
      }
      else if (c=='W')
      {
        if ((c+":"+pos)==last_move)
          board+='<img alt="'+pos+'" src="images/board/white-red.png"/>';
        else
          board+='<img alt="'+pos+'" src="images/board/white.png"/>';
      }
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
      {
        if (isHoshi(x,y))
          board+='<img alt="'+pos+'" src="images/board/hoshi.png"/>';
        else
          board+='<img alt="'+pos+'" src="images/board/center.png"/>';
      }
    }
    board+="<br/>\n";
  }
  $('#board').html(board);
  if (!game_over)
  {
    if (next_color=='B' && (engine_color=='black' || engine_color=='both'))
      doGtpCmdRefresh('genmove',next_color);
    else if (next_color=='W' && (engine_color=='white' || engine_color=='both'))
      doGtpCmdRefresh('genmove',next_color);
    else
    {
      $('#board img').click(function()
      {
        pos=$(this).attr('alt');
        doGtpCmdRefresh('play',next_color+'&'+pos);
      });
    }
  }
}

function updateStatus()
{
  var stat='';

  stat+='Komi: '+komi+'<br/>\n';
  stat+='Moves: '+moves+'<br/>\n';
  //stat+='Last Move: '+last_move+'<br/>\n';
  if (game_over)
    stat+='Game finished<br/>\n';
  else
    stat+='Next to Move: '+next_color+'<br/>\n';
  
  $('#status').html(stat);

  if (game_over)
  {
    if (last_move=='RESIGN')
      $('#status').append('Score: '+next_color+'+R<br/>\n');
    else
    {
      $.get('final_score.gtpcmd',function(data)
      {
        score=data.substr(2);
        if (score=='0')
          score='Jigo';
        $('#status').append('Score: '+score+'<br/>\n');
      });
    }
  }
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
    passes=data['passes'];
    if (passes>=2 || last_move=='RESIGN')
    {
      game_over=true;
      $('#pass, #genmove').button({disabled:true});
    }
    else
    {
      game_over=false;
      $('#pass, #genmove').button({disabled:false});
    }
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
        engine_color=$('#dialog-new-color').val();

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
    $('#dialog-new-color').val(engine_color);
    $('#dialog-new').dialog('open');
  });
  $('#pass').click(function(){doGtpCmdRefresh('play',next_color+'&pass');});
  $('#genmove').click(function(){doGtpCmdRefresh('genmove',next_color);});

});


