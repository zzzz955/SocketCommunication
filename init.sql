-- 기존 인덱스 삭제
DROP INDEX IF EXISTS 
    idx_users_username,
    idx_users_rating,
    idx_gamesessions_lobby,
    idx_gamesessions_status_time,
    idx_rounds_session,
    idx_roundplayers_round_placement,
    idx_roundplayers_user,
    idx_sessionplayers_user,
    idx_sessionplayers_session,
    idx_gameevents_round_time,
    idx_gameevents_player_type,
    idx_gameevents_type_time,
    idx_lobbies_status,
    idx_lobbies_creator,
    idx_lobbyplayers_user,
    idx_lobbyevents_lobby_time,
    idx_lobbyevents_user_type,
    idx_gameevents_jsonb,
    idx_lobbyevents_jsonb;

-- 기존 테이블 삭제
DROP TABLE IF EXISTS LobbyEvents;
DROP TABLE IF EXISTS LobbyPlayers;
DROP TABLE IF EXISTS Lobbies;
DROP TABLE IF EXISTS GameEvents;
DROP TABLE IF EXISTS SessionPlayers;
DROP TABLE IF EXISTS RoundPlayers;
DROP TABLE IF EXISTS GameRounds;
DROP TABLE IF EXISTS GameSessions;
DROP TABLE IF EXISTS Users;

-- 테이블 재생성
-- Users 테이블
CREATE TABLE Users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    total_games INT DEFAULT 0,
    total_wins INT DEFAULT 0,
    rating INT DEFAULT 1000
);

-- Lobbies 테이블 (Users 참조)
CREATE TABLE Lobbies (
    lobby_id SERIAL PRIMARY KEY,
    lobby_code VARCHAR(20) NOT NULL UNIQUE,
    creator_id INT REFERENCES Users(user_id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    closed_at TIMESTAMP,
    max_players INT DEFAULT 6,
    map_id INT,
    game_mode VARCHAR(20),
    status VARCHAR(20) DEFAULT 'open'
);

-- GameSessions 테이블 (Lobbies 참조)
CREATE TABLE GameSessions (
    session_id SERIAL PRIMARY KEY,
    session_code VARCHAR(20) NOT NULL UNIQUE,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP,
    map_id INT,
    game_mode VARCHAR(20),
    total_rounds INT DEFAULT 3,
    lobby_id INT REFERENCES Lobbies(lobby_id),
    status VARCHAR(20) DEFAULT 'active'
);

-- GameRounds 테이블 (GameSessions 참조)
CREATE TABLE GameRounds (
    round_id SERIAL PRIMARY KEY,
    session_id INT REFERENCES GameSessions(session_id),
    round_number INT NOT NULL,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP,
    status VARCHAR(20) DEFAULT 'active',
    UNIQUE (session_id, round_number)
);

-- SessionPlayers 테이블 (GameSessions, Users 참조)
CREATE TABLE SessionPlayers (
    id SERIAL PRIMARY KEY,
    session_id INT REFERENCES GameSessions(session_id),
    user_id INT REFERENCES Users(user_id),
    team_id INT NULL DEFAULT NULL,
    character_id INT NOT NULL,
    join_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    leave_time TIMESTAMP,
    final_score INT,
    final_placement INT,
    old_rating INT,
    new_rating INT,
    rating_change INT,
    UNIQUE (session_id, user_id)
);

-- RoundPlayers 테이블 (GameRounds, Users 참조)
CREATE TABLE RoundPlayers (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES GameRounds(round_id),
    user_id INT REFERENCES Users(user_id),
    placement INT,
    survival_time INT,
    kills INT DEFAULT 0,
    damage_dealt INT DEFAULT 0,
    round_score INT DEFAULT 0,
    is_survived BOOLEAN DEFAULT false,
    UNIQUE (round_id, user_id)
);

-- GameEvents 테이블 (GameRounds, Users 참조)
CREATE TABLE GameEvents (
    event_id SERIAL PRIMARY KEY,
    round_id INT REFERENCES GameRounds(round_id),
    event_type VARCHAR(20) NOT NULL,
    event_time TIMESTAMP NOT NULL,
    player_id INT REFERENCES Users(user_id),
    target_id INT REFERENCES Users(user_id) NULL,
    position_x FLOAT,
    position_y FLOAT,
    position_z FLOAT,
    value INT,
    additional_data JSONB
);

-- LobbyPlayers 테이블 (Lobbies, Users 참조)
CREATE TABLE LobbyPlayers (
    id SERIAL PRIMARY KEY,
    lobby_id INT REFERENCES Lobbies(lobby_id),
    user_id INT REFERENCES Users(user_id),
    join_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    leave_time TIMESTAMP,
    UNIQUE (lobby_id, user_id)
);

-- LobbyEvents 테이블 (Lobbies, Users 참조)
CREATE TABLE LobbyEvents (
    event_id SERIAL PRIMARY KEY,
    lobby_id INT REFERENCES Lobbies(lobby_id),
    event_type VARCHAR(30) NOT NULL,
    event_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_id INT REFERENCES Users(user_id),
    target_id INT REFERENCES Users(user_id) NULL,
    event_data JSONB
);

-- Recreate Indexes
-- Users 테이블 인덱스
CREATE INDEX idx_users_username ON Users(username);
CREATE INDEX idx_users_rating ON Users(rating DESC);

-- GameSessions 테이블 인덱스
CREATE INDEX idx_gamesessions_lobby ON GameSessions(lobby_id);
CREATE INDEX idx_gamesessions_status_time ON GameSessions(status, start_time DESC);

-- GameRounds 테이블 인덱스
CREATE INDEX idx_rounds_session ON GameRounds(session_id);

-- RoundPlayers 테이블 인덱스
CREATE INDEX idx_roundplayers_round_placement ON RoundPlayers(round_id, placement);
CREATE INDEX idx_roundplayers_user ON RoundPlayers(user_id);

-- SessionPlayers 테이블 인덱스
CREATE INDEX idx_sessionplayers_user ON SessionPlayers(user_id);
CREATE INDEX idx_sessionplayers_session ON SessionPlayers(session_id);

-- GameEvents 테이블 인덱스
CREATE INDEX idx_gameevents_round_time ON GameEvents(round_id, event_time);
CREATE INDEX idx_gameevents_player_type ON GameEvents(player_id, event_type);
CREATE INDEX idx_gameevents_type_time ON GameEvents(event_type, event_time);

-- Lobbies 테이블 인덱스
CREATE INDEX idx_lobbies_status ON Lobbies(status, created_at DESC);
CREATE INDEX idx_lobbies_creator ON Lobbies(creator_id);

-- LobbyPlayers 테이블 인덱스
CREATE INDEX idx_lobbyplayers_user ON LobbyPlayers(user_id);

-- LobbyEvents 테이블 인덱스
CREATE INDEX idx_lobbyevents_lobby_time ON LobbyEvents(lobby_id, event_time);
CREATE INDEX idx_lobbyevents_user_type ON LobbyEvents(user_id, event_type);

-- JSONB 인덱스 (PostgreSQL)
CREATE INDEX idx_gameevents_jsonb ON GameEvents USING GIN (additional_data jsonb_path_ops);
CREATE INDEX idx_lobbyevents_jsonb ON LobbyEvents USING GIN (event_data jsonb_path_ops);