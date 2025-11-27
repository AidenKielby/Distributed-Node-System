def maze(w, h):
    import random
    maze = [['#']*w for _ in range(h)]
    x, y = 1,1
    maze[y][x] = ' '
    stack = [(x,y)]
    while stack:
        x, y = stack[-1]
        neighbors = [(x+dx*2, y+dy*2) for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]
                     if 0 < x+dx*2 < w-1 and 0 < y+dy*2 < h-1 and maze[y+dy*2][x+dx*2] == '#']
        if neighbors:
            nx, ny = random.choice(neighbors)
            maze[ny][nx] = ' '
            maze[y + (ny-y)//2][x + (nx-x)//2] = ' '
            stack.append((nx, ny))
        else:
            stack.pop()
    return '\n'.join(''.join(row) for row in maze)