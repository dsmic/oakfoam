
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
var board_data=null;
var board_scored=null;
var spinners=null;
var spinnercount=12;
var spinnerp=2000;
var spinneri=0;
var spinnertick=null;
var buttons={};
var buttons_enabled={};
var info='';

var icons=
{
  'new':'M23.024,5.673c-1.744-1.694-3.625-3.051-5.168-3.236c-0.084-0.012-0.171-0.019-0.263-0.021H7.438c-0.162,0-0.322,0.063-0.436,0.18C6.889,2.71,6.822,2.87,6.822,3.033v25.75c0,0.162,0.063,0.317,0.18,0.435c0.117,0.116,0.271,0.179,0.436,0.179h18.364c0.162,0,0.317-0.062,0.434-0.179c0.117-0.117,0.182-0.272,0.182-0.435V11.648C26.382,9.659,24.824,7.49,23.024,5.673zM25.184,28.164H8.052V3.646h9.542v0.002c0.416-0.025,0.775,0.386,1.05,1.326c0.25,0.895,0.313,2.062,0.312,2.871c0.002,0.593-0.027,0.991-0.027,0.991l-0.049,0.652l0.656,0.007c0.003,0,1.516,0.018,3,0.355c1.426,0.308,2.541,0.922,2.645,1.617c0.004,0.062,0.005,0.124,0.004,0.182V28.164z',
  'pass':'M29.342,15.5l-7.556-4.363v2.614H18.75c-1.441-0.004-2.423,1.002-2.875,1.784c-0.735,1.222-1.056,2.561-1.441,3.522c-0.135,0.361-0.278,0.655-0.376,0.817c-1.626,0-0.998,0-2.768,0c-0.213-0.398-0.571-1.557-0.923-2.692c-0.237-0.676-0.5-1.381-1.013-2.071C8.878,14.43,7.89,13.726,6.75,13.75H2.812v3.499c0,0,0.358,0,1.031,0h2.741c0.008,0.013,0.018,0.028,0.029,0.046c0.291,0.401,0.634,1.663,1.031,2.888c0.218,0.623,0.455,1.262,0.92,1.897c0.417,0.614,1.319,1.293,2.383,1.293H11c2.25,0,1.249,0,3.374,0c0.696,0.01,1.371-0.286,1.809-0.657c1.439-1.338,1.608-2.886,2.13-4.127c0.218-0.608,0.453-1.115,0.605-1.314c0.006-0.01,0.012-0.018,0.018-0.025h2.85v2.614L29.342,15.5zM10.173,14.539c0.568,0.76,0.874,1.559,1.137,2.311c0.04,0.128,0.082,0.264,0.125,0.399h2.58c0.246-0.697,0.553-1.479,1.005-2.228c0.252-0.438,0.621-0.887,1.08-1.272H9.43C9.735,14.003,9.99,14.277,10.173,14.539z',
  'genmove':'M6.684,25.682L24.316,15.5L6.684,5.318V25.682z',
  'stop':'M5.5,5.5h20v20h-20z',
  'info':'M16,1.466C7.973,1.466,1.466,7.973,1.466,16c0,8.027,6.507,14.534,14.534,14.534c8.027,0,14.534-6.507,14.534-14.534C30.534,7.973,24.027,1.466,16,1.466z M14.757,8h2.42v2.574h-2.42V8z M18.762,23.622H16.1c-1.034,0-1.475-0.44-1.475-1.496v-6.865c0-0.33-0.176-0.484-0.484-0.484h-0.88V12.4h2.662c1.035,0,1.474,0.462,1.474,1.496v6.887c0,0.309,0.176,0.484,0.484,0.484h0.88V23.622z',
};

function xy2pos(x,y)
{
  if (x>7) x++; // there is no I column
  return String.fromCharCode('A'.charCodeAt()+x)+(y+1);
}

function doGtpCmdThink(cmd,args,func)
{
  if (!thinking)
  {
    thinking=true;
    spinnersStart();
  }
  drawBoard();
  disableButton('pass');
  disableButton('genmove');
  enableButton('stop');
  $('#status').html('Engine is thinking...');
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

function spinnersStart()
{
  (function ticker() {
    if (!thinking)
      return;
    n=spinnercount;
    for (i=0;i<n;i++) {
      j=(spinneri+i)%n;
      spinners[j].attr('opacity',i/n);
    }
    spinneri=(spinneri+1)%n;
    spinnertick=setTimeout(ticker,spinnerp/n);
  })();
}

function drawBoard()
{
  var data=board_data;
  var sz=board_size*24;
  var lw=1.5;
  var crad=11;
  var bdr=4;
  if (size_chg)
  {
    if (paper!=null)
    {
      paper.remove();
      for (i=0;i<n;i++)
      {
        spinners[i].remove();
      }
    }
    width=board_size*24+45+2*bdr;
    $('#page').width(width); // dynamic size
    $('#status').width(width-5);
    paper=Raphael('board',sz+2*bdr,sz+2*bdr);

    n=spinnercount;
    csz=sz/(n*2.5);
    cxy=sz/2+bdr;
    spinners=[];
    for (i=0;i<spinnercount;i++)
    {
      alpha=i*Math.PI*2/n;
      x=0.15*sz*Math.cos(alpha);
      y=0.15*sz*Math.sin(alpha);
      spinners[i]=paper.circle(cxy+x,cxy+y,csz).attr({fill:'#fff',stroke:'none',opacity:0});
    }

    spinnersStart();
    size_chg=false;
  }

  paper.rect(1,1,sz-2+2*bdr,sz-2+2*bdr).attr({fill:'#c8a567', stroke:'#352b17', 'stroke-width':1});
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
          paper.circle(x*24+12+bdr,y*24+12+bdr,crad).attr({fill:'r(.3,.2)#555-#000'});
        else if (c=='W')
          paper.circle(x*24+12+bdr,y*24+12+bdr,crad).attr({fill:'r(.3,.2)#fff-#aaa',stroke:'#777'});

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
            hvo=paper.circle(x,y,crad).attr({stroke:'none',opacity:0.5});
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
  
  if (game_over && board_scored!=null)
  {
    for (x=0;x<board_size;x++)
    {
      for (y=0;y<board_size;y++)
      {
        pos=xy2pos(x,y)
        c=board_scored[pos];
        if (c=='B')
          paper.rect(x*24+bdr,y*24+bdr,24,24).attr({fill:'#000',stroke:'none',opacity:0.4});
        else if (c=='W')
          paper.rect(x*24+bdr,y*24+bdr,24,24).attr({fill:'#fff',stroke:'none',opacity:0.4});
      }
    }
  }


  if (thinking)
  {
    paper.rect(1,1,sz-2+2*bdr,sz-2+2*bdr).attr({fill:'#777',stroke:'none',opacity:0.4});
    if (thinking)
    {
      for (i=0;i<spinnercount;i++)
      {
        spinners[i].toFront();
      }
    }
  }
  else
  {
    for (i=0;i<spinnercount;i++)
    {
      spinners[i].toBack();
    }
  }
}

function moveDone()
{
  if (!game_over)
  {
    if (next_color=='B' && (engine_color=='black' || engine_color=='both'))
      doGtpCmdThink('genmove',next_color,refreshBoard);
    else if (next_color=='W' && (engine_color=='white' || engine_color=='both'))
      doGtpCmdThink('genmove',next_color,refreshBoard);
    else
    {
      thinking=false;
      enableButton('pass');
      enableButton('genmove');
      disableButton('stop');
      drawBoard();
    }
  }
  else
  {
    thinking=false;
    drawBoard();
  }
}

function updateStatus()
{
  info='';
  info+='Komi: '+komi+'<br/>\n';
  info+='Moves: '+moves+'<br/>\n';
  //info+='Last Move: '+last_move+'<br/>\n';
  /*if (moves>0 && last_move.split(':')[1]=='PASS')
  {
    if (last_move.split(':')[0]=='B')
      info+='Black';
    else
      info+='White';
    info+=' passed<br/>\n';
  }*/
  if (game_over)
    info+='Game finished<br/>\n';
  else
    info+='Next to Move: '+next_color+'<br/>\n';
  
  stat=moves+': ';
  if (moves>0)
    stat+=last_move.split(':')[1];
  $('#status').html(stat);

  if (game_over)
  {
    if (last_move=='RESIGN')
    {
      info+='Result: '+next_color+'+R<br/>\n';
      $('#status').append(' Result: '+next_color+'+R');
    }
    else
    {
      $.get('final_score.gtpcmd',function(data)
      {
        score=data.substr(2);
        if (score=='0')
          score='Jigo';
        info+='Result: '+score+'<br/>\n';
        $('#status').append(' Result: '+score);
        $.getJSON('board_scored.jsoncmd',function(data)
        {
          board_scored=data;
          drawBoard();
        });
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
    {
      game_over=false;
      board_scored=null;
    }

    if (thinking || game_over)
    {
      disableButton('pass');
      disableButton('genmove');
    }
    else
    {
      enableButton('pass');
      enableButton('genmove');
    }
    
    if (thinking)
      enableButton('stop');
    else
      disableButton('stop');

    updateStatus();
    $.getJSON('board_pos.jsoncmd',function(data)
    {
      board_data=data;
      drawBoard();
      moveDone();
    });
  });
}

function initButton(name)
{
  r=Raphael(name,32,32);
  buttons[name]=r.path(icons[name]).attr({fill:'#000',stroke:'none',transform:'s0.8'});
  hvo=r.rect(0,0,32,32).attr({'fill':'#000','opacity':0});
  hvo.mouseover(function(){if (buttons_enabled[name]) buttons[name].attr('fill','#333');});
  hvo.mouseout(function(){if (buttons_enabled[name]) buttons[name].attr('fill','#000');});
  buttons_enabled[name]=true;
}

function disableButton(name)
{
  buttons[name].attr('fill','#777');
  buttons_enabled[name]=false;
}

function enableButton(name)
{
  buttons[name].attr('fill','#000');
  buttons_enabled[name]=true;
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
  
  initButton('new');
  initButton('pass');
  initButton('genmove');
  initButton('stop');
  initButton('info');
  $('#menu').tooltip();

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
        size_chg=true;
        engine_color=$('#dialog-new-color').val();
        
        enableButton('info');

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
  $('#dialog-info').dialog(
  {
    autoOpen: false,
    width: 250,
    modal: true,
    resizable: false,
    buttons: {
      'OK': function()
      {
        $(this).dialog('close');
      }
    }
  });

  $('#new').click(function()
  {
    $('#dialog-new-size').val(board_size);
    $('#dialog-new-komi').val(komi);
    $('#dialog-new-color').val(engine_color);
    $('#dialog-new').dialog('open');
  });
  $('#pass').click(function(){if (!buttons_enabled['pass']) return; doGtpCmd('play',next_color+'&pass',refreshBoard);});
  $('#genmove').click(function(){if (!buttons_enabled['genmove']) return; doGtpCmdThink('genmove',next_color,refreshBoard);});
  $('#stop').click(function()
  {
    if (!buttons_enabled['stop']) return;
    disableButton('stop');
    doGtpCmd('stop','',function()
    {
      engine_color='none'
      refreshBoard();
    });
  });
  $('#info').click(function()
  {
    if (!buttons_enabled['info']) return;
    $('#dialog-info-status').html(info);
    $('#dialog-info').dialog('open');
  });

  refreshBoard();
});


