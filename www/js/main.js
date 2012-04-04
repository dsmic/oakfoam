var board_size;
var komi;
var moves;
var last_move;
var next_color;
var engine_color='none';
var passes;
var game_over=false;
var paper=null;
var size_chg=true;
var thinking=false;

function xy2pos(x,y)
{
  if (x>7) x++; // there is no I column
  return String.fromCharCode('A'.charCodeAt()+x)+(y+1);
}

function doGtpCmdThink(cmd,args,func)
{
  thinking=true;
  $('#pass, #genmove').button({disabled:true});
  $('#status').append('Thinking...<br/>\n');
  doGtpCmd(cmd,args,func)
}

function doGtpCmd(cmd,args,func)
{
  $.get(cmd+'.gtpcmd?'+args,func);
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
  var sz=board_size*24;
  var lw=2;
  var crad=11;
  var bdr=4;
  if (size_chg)
  {
    if (paper!=null)
      paper.remove();
    paper=Raphael('board',sz+2*bdr,sz+2*bdr);
    size_chg=false;
  }
  paper.rect(1,1,sz-2+2*bdr,sz-2+2*bdr).attr({fill:'#c4a055', stroke:'#352b17', 'stroke-width':1});
  for (i=0;i<board_size;i++)
  {
    paper.rect(i*24+12-lw/2+bdr,12-lw/2+bdr,lw,sz-24+lw).attr({fill:'#000000', stroke:'none'});
    paper.rect(12-lw/2+bdr,i*24+12-lw/2+bdr,sz-24+lw,lw).attr({fill:'#000000', stroke:'none'});
  }
  for (x=0;x<board_size;x++)
  {
    for (y=0;y<board_size;y++)
    {
      if (isHoshi(x,y))
        paper.circle(x*24+12+bdr,y*24+12+bdr,4).attr({fill:'#000000', stroke:'none'});
    }
  }
  for (x=0;x<board_size;x++)
  {
    for (y=0;y<board_size;y++)
    {
      pos=xy2pos(x,y)
      c=data[pos];

      if (c=='B' || c=='W')
      {
        if (c=='B')
          paper.circle(x*24+12+bdr,y*24+12+bdr,crad).attr({fill:'#000000'});
        else if (c=='W')
          paper.circle(x*24+12+bdr,y*24+12+bdr,crad).attr({fill:'#ffffff',stroke:'#333333'});

        if ((c+":"+pos)==last_move)
          paper.circle(x*24+12+bdr,y*24+12+bdr,4).attr({fill:'#7f7f7f',stroke:'none'});
      }
      else
      {
        itt=paper.rect(x*24+bdr,y*24+bdr,24,24).attr({fill:'#000',opacity:0});
        itt.data('pos',pos);
        itt.data('hvx',x*24+bdr+12);
        itt.data('hvy',y*24+bdr+12);
        itt.data('hvo',null);

        engine_to_play=engine_color=='both';
        if (next_color=='B' && engine_color=='black')
          engine_to_play=true;
        else if (next_color=='W' && engine_color=='white')
          engine_to_play=true;

        if (!game_over && !engine_to_play)
        {
          itt.click(function()
          {
            doGtpCmd('play',next_color+'&'+this.data('pos'),refreshBoard);
          });
          itt.mouseover(function()
          {
            x=this.data('hvx');
            y=this.data('hvy');
            hvo=paper.circle(x,y,crad).attr({opacity:0.5});
            if (next_color=='B')
              hvo.attr({fill:'#000000'});
            else
              hvo.attr({fill:'#ffffff'});
            this.data('hvo',hvo);
            this.toFront();
          });
          itt.mouseout(function()
          {
            o=this.data('hvo');
            if (o!=null)
              o.remove();
          });
        }
      }
    }
  }

  if (!game_over)
  {
    if (next_color=='B' && (engine_color=='black' || engine_color=='both'))
      doGtpCmdThink('genmove',next_color,refreshBoard);
    else if (next_color=='W' && (engine_color=='white' || engine_color=='both'))
      doGtpCmdThink('genmove',next_color,refreshBoard);
    else
    {
      thinking=false;
      $('#pass, #genmove').button({disabled:false});
    }
  }
  $('#page').width(board_size*24+200+2*bdr); // dynamic size
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
    if (data['size']!=board_size)
      size_chg=true;
    board_size=data['size'];
    komi=data['komi'];
    moves=data['moves'];
    last_move=data['last_move'];
    next_color=data['next_color'];
    passes=data['passes'];

    if (passes>=2 || last_move=='RESIGN')
      game_over=true;
    else
      game_over=false;

    if (thinking || game_over)
      $('#pass, #genmove').button({disabled:true});
    else
      $('#pass, #genmove').button({disabled:false});

    updateStatus();
    $.getJSON('board_pos.jsoncmd',function(data){drawBoard(data);});
  });
}

$(document).ready(function()
{
  $.getJSON('engine_info.jsoncmd',function(data)
  {
    $('#engine').html(data['name']+' '+data['version']);
  });
  
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
        engine_color=$('#dialog-new-color').val();
        doGtpCmd('clear_board','',function(){
          doGtpCmd('boardsize',$('#dialog-new-size').val(),function(){
            doGtpCmd('komi',$('#dialog-new-komi').val(),refreshBoard);
          });
        });

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
  $('#pass').click(function(){doGtpCmd('play',next_color+'&pass',refreshBoard);});
  $('#genmove').click(function(){doGtpCmdThink('genmove',next_color,refreshBoard);});

  refreshBoard();
});


