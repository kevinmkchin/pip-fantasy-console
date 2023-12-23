title = "Pong"
author = "Kevin Chin"

player1 = { x : 0, y : 0 }
player2 = { x: 0, y: 0 }
ball = { x: 0, y: 0, r: 10, col = { x: -10, y: -10, w: 20, h: 20 } }

fn init ()
{
  print("game start")
}

fn tick () 
{
  DoInputAndMovePlayer1()
  DoInputAndMovePlayer2()
		MoveBall() 
	 ResolveBallToPlayerCollision()
}

fn draw () 
{
  gfx.rect() ;; player1
  gfx.rect() ;; player2
  gfx.circle(ball.x, ball.y, ball.r)
}

